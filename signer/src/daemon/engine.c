/*
 * $Id$
 *
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * The engine.
 *
 */

#include "daemon/engine.h"

#include <malloc.h>
#include <time.h>
#include <stdio.h>

/**
 * Create engine.
 *
 */
engine_type*
engine_create(void)
{
    engine_type* engine = (engine_type*) malloc(sizeof(engine_type));

    engine->daemonize = 0;
    engine->need_to_exit = 0;
    engine->need_to_reload = 0;

    return engine;
}


/**
 * Start engine.
 *
 */
void
engine_start(const char* cfgfile, int cmdline_verbosity, int daemonize,
    int info)
{
    engine_type* engine = NULL;

    fprintf(stdout, "start engine");

    /* initialize */
    engine = engine_create();
    engine->daemonize = daemonize;

    /* configure */

    if (info) {
        /* print info */
        return;
    }

    /* setup */
    tzset(); /* for portability */

    /* run */
    while (engine->need_to_exit == 0) {
        engine->need_to_exit = 1;
    }
    /* shutdown */

    fprintf(stdout, "shutdown engine");

    engine_cleanup(engine);
    return;
}

/**
 * Clean up engine.
 *
 */
void
engine_cleanup(engine_type* engine)
{
    if (engine) {
        free((void*) engine);
    }
    return;
}
