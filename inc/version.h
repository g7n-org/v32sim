#ifndef  _VERSION_H
#define  _VERSION_H

////////////////////////////////////////////////////////////////////////////////////////
//
// v32sim version string. This is the single source of truth: the --version option,
// the startup banner, the 'version' prompt command, and `make version` all derive from
// this value. Release convention is YYYYMMDD, with a '-dev' suffix on development
// builds. It may be overridden at build time, eg: make CFLAGS_EXTRA='-DVERSION=\"x\"'
//
#ifndef  VERSION
#define  VERSION                 "20261025-dev"
#endif

#endif
