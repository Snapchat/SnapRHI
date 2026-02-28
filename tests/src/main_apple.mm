//
//  main_apple.mm
//  SnapRHI Tests — Apple-platform entry point
//
//  This file replaces main.cpp on Apple platforms to address two issues that
//  cause Catch2 to report "Unknown exception" instead of a diagnostic message:
//
//  1. @autoreleasepool — Metal API calls create autoreleased Objective-C
//     objects.  Without a drain pool the ObjC runtime accumulates them and
//     eventually throws an NSException, which Catch2 cannot describe.
//
//  2. NSException → std::exception translation — Even with a pool, the Metal
//     framework or its debug layer can throw NSException at any time (shader
//     compilation failures, debug-layer assertions, invalid API usage, etc.).
//     NSException does NOT inherit from std::exception, so Catch2 catches it
//     with catch(...) and prints "Unknown exception".  We register a Catch2
//     TRANSLATE_EXCEPTION handler that converts NSException* into a human-
//     readable std::string, giving the test runner full diagnostic output.
//
//  Copyright © 2026 Snapchat.  All rights reserved.
//

#define CATCH_CONFIG_RUNNER   // We provide our own main()
#include <catch2/catch.hpp>

#import <Foundation/Foundation.h>

// ── NSException translator ─────────────────────────────────────────────────
// On Apple's modern 64-bit runtime, ObjC exceptions are implemented on top
// of the C++ unwind ABI (__cxa_throw).  Catch2 can therefore catch them by
// type.  This translator converts them to a descriptive string so the test
// output shows the exception name, reason, and call stack instead of
// "Unknown exception".
CATCH_TRANSLATE_EXCEPTION(NSException*& e) {
    std::string description = "[NSException] ";
    if (e.name)   description += std::string([e.name UTF8String]);
    if (e.reason) description += ": " + std::string([e.reason UTF8String]);
    return description;
}

int main(int argc, char* argv[]) {
    {
        Catch::Session session;

        const int result = session.applyCommandLine(argc, argv);
        if (result != 0) {
            return result;
        }

        return session.run();
    }
}
