// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ASSERT_INCLUDED
#define GPC_ASSERT_INCLUDED 1

// ----------------------------------------------------------------------------
//
//          CORE API
//
// ----------------------------------------------------------------------------

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test.
void gpc_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite.
void gpc_suite(const char* name);

// ----------------------------------------------------------------------------
//
//          END OF CORE API
//
// ----------------------------------------------------------------------------

#endif // GPC_ASSERT_INCLUDED
