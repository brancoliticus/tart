/*

effect.h -- Tiny Actor Run-Time

"MIT License"

Copyright (c) 2013 Dale Schumacher, Tristan Slominski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "tart.h"
#include "pair.h"
#include "actor.h"

typedef struct effect EFFECT, *Effect;

struct effect {
    Config      sponsor;    // sponsor configuration
    Actor       self;       // currently active (meta-)actor
    Pair        actors;     // list of actors created
    Pair        events;     // list of messages sent
    Behavior    behavior;   // replacement behavior
};

extern Effect   effect_new(Config cfg, Actor a);
extern void     effect_send(Effect fx, Actor target, Any msg);
extern void     effect_create(Effect fx, Behavior beh);
extern void     effect_become(Effect fx, Behavior beh);
extern void     effect_commit(Effect fx);

extern void     act_busy(Event e);
extern void     act_begin(Event e);
extern void     act_send(Event e);
extern void     act_create(Event e);
extern void     act_become(Event e);
extern void     act_commit(Event e);

extern BEHAVIOR busy_behavior;
extern BEHAVIOR commit_behavior;
extern ACTOR commit_actor;

#endif /* _EFFECT_H_ */