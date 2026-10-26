// refrun: run a BIOS + cartridge on the stock Vircon32 ConsoleLogic, headless.
//
//   refrun BIOS.v32 CART.v32 FRAMES [--date Y,D] [--time S] [--dump N@HEXADDR] [--pad]
//
// The console logic (CPU, buses, timer, RNG, GPU/SPU state) is the DesktopEmulator's own,
// compiled unchanged; only the video, audio and log callbacks are stubbed. After the run
// it prints N RAM words, one signed integer per line, like `v32sim --dump`. That makes it
// the oracle for differential tests of v32sim (tests/run_gaps.sh).
//
// Build: see tests/ref/build.sh
#include "ExternalInterfaces.hpp"
#include "V32Console.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace std;

namespace V32
{
    namespace Callbacks
    {
        static void NoColor( GPUColor ) {}
        static void NoQuad( GPUQuad& ) {}
        static void NoInt( int ) {}
        static void NoLoad( int, void* ) {}
        static void NoVoid() {}
        static void Log( const string& ) {}
        static void Throw( const string& Message )
        {
            fprintf( stderr, "refrun: %s\n", Message.c_str() );
            exit( 3 );
        }

        struct Install
        {
            Install()
            {
                ClearScreen = NoColor;
                DrawQuad = NoQuad;
                SetMultiplyColor = NoColor;
                SetBlendingMode = NoInt;
                SelectTexture = NoInt;
                LoadTexture = NoLoad;
                UnloadCartridgeTextures = NoVoid;
                UnloadBiosTexture = NoVoid;
                LogLine = Log;
                ThrowException = Throw;
            }
        };
        static Install Installer;
    }
}

int main( int argc, char** argv )
{
    if( argc < 4 )
    {
        fprintf( stderr, "usage: refrun BIOS CART FRAMES [--date Y,D] [--time S] [--dump N@HEXADDR]\n" );
        return 2;
    }

    int Frames = atoi( argv[ 3 ] );
    int Year = -1, Day = 0, Seconds = -1;
    int DumpCount = 0;
    unsigned DumpAddress = 0;

    for( int i = 4; i < argc; i++ )
    {
        if( !strcmp( argv[ i ], "--date" ) && i + 1 < argc )
            sscanf( argv[ ++i ], "%d,%d", &Year, &Day );
        else if( !strcmp( argv[ i ], "--time" ) && i + 1 < argc )
            Seconds = atoi( argv[ ++i ] );
        else if( !strcmp( argv[ i ], "--dump" ) && i + 1 < argc )
            sscanf( argv[ ++i ], "%d@%x", &DumpCount, &DumpAddress );
    }

    V32::V32Console Console;
    Console.LoadBios( argv[ 1 ] );
    Console.LoadCartridge( argv[ 2 ] );
    if( Year >= 0 )
        Console.SetCurrentDate( Year, Day );
    if( Seconds >= 0 )
        Console.SetCurrentTime( Seconds / 3600, (Seconds / 60) % 60, Seconds % 60 );

    Console.SetPower( true );

    for( int f = 0; f < Frames; f++ )
    {
        Console.RunNextFrame();
        if( Console.IsCPUHalted() )
            break;
    }

    for( int i = 0; i < DumpCount; i++ )
    {
        unsigned Address = DumpAddress + i;
        if( Address >= (unsigned)Console.RAM.MemorySize )
            break;
        printf( "%d\n", (int)Console.RAM.Memory[ Address ].AsInteger );
    }

    return 0;
}
