# Bug reports for the Vircon32 DesktopEmulator

Three code defects found while comparing v32sim against the DesktopEmulator
(`ComputerSoftware/DesktopEmulator/ConsoleLogic`). Line numbers are for commit
`ea62bc7caad34f483945eadb114cf7d9c26cf54c` of `wedge1020/ComputerSoftware`; please
re-check them against `vircon32/ComputerSoftware` before filing, since the upstream
tree may have moved.

**How these were found.** By reading the source; none was reproduced by running the
emulator. Each report says what the code does, what was almost certainly intended, and
how to check. Summary:

| #  | Component | File                      | Lines     | Severity   | Observable? |
| -- | --------- | ------------------------- | --------- | ---------- | ----------- |
| 1  | GPU       | `V32GPU.cpp`              | 195, 206  | Low        | after a console reset, for regions a program never defines |
| 2  | GPU port  | `V32GPUWriters.cpp`       | 112-114   | Cosmetic   | no (dead code, no visible effect) |
| 3  | SPU       | `V32SPU.cpp`              | 222, 231  | Low (latent) | not with the current callers |

---

## 1. `V32GPU::Reset` never clears region `MinY`

**File:** `V32GPU.cpp`, `V32GPU::Reset()`, lines 195-196 (cartridge textures) and
206-207 (BIOS texture).

**Code:**

```cpp
for( int i = 0; i < Constants::GPUMaximumCartridgeTextures; i++ )
for( int j = 0; j < Constants::GPURegionsPerTexture; j++ )
{
    CartridgeTextures[ i ].Regions[ j ].MinX     = 0;
    CartridgeTextures[ i ].Regions[ j ].MinX     = 0;      // <-- duplicate
    CartridgeTextures[ i ].Regions[ j ].MaxX     = 0;
    CartridgeTextures[ i ].Regions[ j ].MaxY     = 0;
    CartridgeTextures[ i ].Regions[ j ].HotspotX = 0;
    CartridgeTextures[ i ].Regions[ j ].HotspotY = 0;
}

// same for BIOS texture
for( int j = 0; j < Constants::GPURegionsPerTexture; j++ )
{
    BiosTexture.Regions[ j ].MinX     = 0;
    BiosTexture.Regions[ j ].MinX     = 0;                 // <-- duplicate
    ...
}
```

`MinX` is assigned twice and `MinY` never, in both loops. This reads as a copy-paste slip
(the second line was meant to be `MinY`).

**Effect.** `Reset()` is meant to return every region to zero ("reset all regions for every
textures"). Because `MinY` is skipped, a region keeps its previous `MinY` across a console
reset while its other five fields are cleared. This is invisible at first power-on (the
tables start zeroed, to be confirmed) and for any program that defines all fields of the
regions it draws, which is why it has gone unnoticed. It is visible to a program that
reads `GPU_RegionMinY`, or draws, a region it never defined, after a reset: the region
has a stale top edge and its other fields are zero, so it draws a different rectangle than
on a clean start.

**Severity:** Low. Wrong state after reset, but only for undefined regions.

**To reproduce.** On a cartridge program, using BIOS texture regions that the BIOS font does
not use (for example region 4000 of texture -1):

```
    out  GPU_SelectedTexture, -1
    out  GPU_SelectedRegion,  4000
    out  GPU_RegionMinY,      7
    ; ... reset the console from the emulator menu, run the program again, but this time
    ; only read, without writing:
    out  GPU_SelectedTexture, -1
    out  GPU_SelectedRegion,  4000
    in   R0, GPU_RegionMinY      ; expected 0 after a reset; the code above leaves 7
```

**Suggested fix.** Replace the duplicate line in both loops:

```diff
-            CartridgeTextures[ i ].Regions[ j ].MinX     = 0;
+            CartridgeTextures[ i ].Regions[ j ].MinY     = 0;
...
-            BiosTexture.Regions[ j ].MinX     = 0;
+            BiosTexture.Regions[ j ].MinY     = 0;
```

(applied to the second occurrence in each loop: lines 196 and 207.)

---

## 2. `WriteGPUActiveBlending`: unreachable "ignore unknown values" return

**File:** `V32GPUWriters.cpp`, `WriteGPUActiveBlending()`, lines 90-120 (the stray lines
are 112-114).

**Code:**

```cpp
switch( Value.AsInteger )
{
    case (int32_t)IOPortValues::GPUBlendingMode_Alpha:    { GPU.ActiveBlending = Value.AsInteger; break; }
    case (int32_t)IOPortValues::GPUBlendingMode_Add:      { GPU.ActiveBlending = Value.AsInteger; break; }
    case (int32_t)IOPortValues::GPUBlendingMode_Subtract: { GPU.ActiveBlending = Value.AsInteger; break; }

    // unknown blending mode codes are just ignored
    // (the value is not written either)
    return true;                      // <-- inside the switch, after the last break:
}                                     //     no label, so this can never execute

// for valid modes, notify the video library
Callbacks::SetBlendingMode( Value.AsInteger );
return true;
```

The comment says unknown codes return immediately without notifying the video library
(compare the neighboring writers, which use `default:` for this). The `return true;` has no
`default:` label, so it is dead code. An unknown value instead falls out of the `switch` and
reaches `Callbacks::SetBlendingMode( Value.AsInteger )`.

**Effect.** None that a program can see. The port keeps its old value (nothing writes it).
`VideoOutput::SetBlendingMode` (`Emulator/VideoOutput.cpp`, line 795) flushes the queued
quads (`RenderQuadQueue()`) and then ignores the unknown value in its own `default: return;`,
so the OpenGL state is unchanged. The only difference from the documented intent is that
the pending quad queue is flushed early, which cannot change the rendered picture. (The
comment documents the intended behavior, so a future change to `SetBlendingMode` that did
not ignore bad values would turn this into a real bug.)

**Severity:** Cosmetic. Dead code and a misleading comment.

**To check.** `OUT GPU_ActiveBlending, 0x99` followed by `IN R0, GPU_ActiveBlending` gives
the previous mode (0x20 after reset) either way.

**Suggested fix.** Use a `default:` label, as the comment describes:

```diff
             case (int32_t)IOPortValues::GPUBlendingMode_Subtract:
             {
                 GPU.ActiveBlending = Value.AsInteger;
                 break;
             }
 
-            // unknown blending mode codes are just ignored
-            // (the value is not written either)
-            return true;
+            // unknown blending mode codes are just ignored
+            // (the value is not written either)
+            default:
+                return true;
         }
```

---

## 3. `V32SPU::PlayChannel` looks up the sound of the wrong channel

**File:** `V32SPU.cpp`, `V32SPU::PlayChannel( SPUChannel& TargetChannel )`, lines 222 and
231.

**Code:**

```cpp
void V32SPU::PlayChannel( SPUChannel& TargetChannel )
{
    // case 1: for a stopped channel, set the initial play
    if( TargetChannel.State == IOPortValues::SPUChannelState_Stopped )
    {
        SPUSound* ChannelSound = GetChannelSound( PointedChannel );      // <-- line 222
        TargetChannel.LoopEnabled = ChannelSound->PlayWithLoop;
        TargetChannel.Position = 0;
    }
    // case 2: if channel was already playing, make it retrigger
    else if( TargetChannel.State == IOPortValues::SPUChannelState_Playing )
    {
        SPUSound* ChannelSound = GetChannelSound( PointedChannel );      // <-- line 231
        ...
```

The function operates on its parameter `TargetChannel`, but fetches the sound through the
member `PointedChannel` (the channel currently selected through `SPU_SelectedChannel`).
Everything else in the function uses `TargetChannel`; `GetChannelSound( &TargetChannel )`
was clearly meant.

**Effect.** None today. The function has two callers:

  * `WriteSPUCommand`, `SPUCommand_PlaySelectedChannel` (`V32SPUWriters.cpp`, line 27)
    calls `PlayChannel( *SPU.PointedChannel )`: target and pointed channel are the same.
  * `V32SPU::ResumeAllChannels` (`V32SPU.cpp`, line 276) calls it for *paused* channels
    only, which take the third (resume) path that never looks the sound up.

So the two code paths that use the lookup are only reached with
`TargetChannel == *PointedChannel`. The defect would produce a wrong loop flag, taken from
whichever channel is selected, for any future caller that plays a channel other than the
selected one (for example a "play all"/"restart all" command, or a refactor of
`ResumeAllChannels`).

**Severity:** Low (latent). Correct output today; a trap for future changes.

**To check.** A test cannot fail today. To see it, temporarily call
`PlayChannel( Channels[ 3 ] )` for a stopped channel while channel 0 is selected and
assigned to a sound whose `PlayWithLoop` differs from channel 3's sound: channel 3 receives
channel 0's loop setting.

**Suggested fix.**

```diff
-            SPUSound* ChannelSound = GetChannelSound( PointedChannel );
+            SPUSound* ChannelSound = GetChannelSound( &TargetChannel );
```

(both occurrences, lines 222 and 231).

---

## Notes for the reporter

  * Items 1 and 3 are one-token fixes; item 2 is a clean-up. They are independent, so they
    can be filed as three issues or as one patch.
  * Related, but a behavior note rather than a bug: the console's leap year test
    (`V32Timer.cpp`, `V32Timer::ChangeFrame`) is `(Year % 4 == 0) && (Year % 100 != 0)`
    and omits the 400-year rule, so the year 2000 is treated as a non-leap year. If that is
    unintended it is a fourth, trivial report (`|| (Year % 400 == 0)`); v32sim currently
    plans to mirror the stock behavior (see `V32SIMGAPS.md`, G4).
