/*

actor.c -- Tiny Actor Run-Time

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

#include "actor.h"
#include "expr.h"
#include "number.h"

PAIR the_nil_pair_actor = { { beh_pair }, NIL, NIL };
inline Actor
pair_new(Actor h, Actor t)
{
    Pair p = NEW(PAIR);
    BEH(p) = beh_pair;
    p->h = h;
    p->t = t;
    return (Actor)p;
}

inline Actor
list_new()
{
    return a_empty_list;
}
inline Actor
list_empty_p(Actor list)
{
    return ((list == a_empty_list) ? a_true : a_false);
}
inline Pair
list_pop(Actor list)  // returns: (first, rest)
{
    if (beh_pair != BEH(list)) { halt("list_pop: pair required"); }
    return (Pair)list;
}
inline Actor
list_push(Actor list, Actor item)
{
    if (beh_pair != BEH(list)) { halt("list_push: pair required"); }
    return PR(item, list);
}

inline Actor
deque_new()
{
    Actor a = PR(NIL, NIL);
    BEH(a) = beh_deque;  // override pair behavior with deque behavior
    return a;
}
inline Actor
deque_empty_p(Actor queue)
{
    if (beh_deque != BEH(queue)) { halt("deque_empty_p: deque required"); }
    Pair q = (Pair)queue;
    return ((q->h == NIL) ? a_true : a_false);
}
inline void
deque_give(Actor queue, Actor item)
{
    if (beh_deque != BEH(queue)) { halt("deque_give: deque required"); }
    Pair q = (Pair)queue;
    Actor p = PR(item, NIL);
    if (q->h == NIL) {
        q->h = p;
    } else {
        Pair t = (Pair)(q->t);
        t->t = p;
    }
    q->t = p;
}
inline Actor
deque_take(Actor queue)
{
    if (deque_empty_p(queue) != a_false) { halt("deque_take from empty!"); }
//    if (beh_deque != BEH(queue)) { halt("deque_take: deque required"); }
    Pair q = (Pair)queue;
    Pair p = (Pair)(q->h);
    Actor item = p->h;
    q->h = p->t;
    FREE(p);
    return item;
}
inline void
deque_return(Actor queue, Actor item)
{
    if (beh_deque != BEH(queue)) { halt("deque_return: deque required"); }
    Pair q = (Pair)queue;
    Actor p = PR(item, q->h);
    if (q->h == NIL) {
        q->t = p;
    }
    q->h = p;
}
inline Actor
deque_lookup(Actor queue, Actor index)
{
    int i;

    if (beh_integer != BEH(index)) { halt("deque_lookup: index must be an integer"); }
    i = ((Integer)index)->i;
    if (beh_deque != BEH(queue)) { halt("deque_lookup: deque required"); }
    Pair q = (Pair)queue;
    Actor a = q->h;
    while (a != NIL) {
        if (beh_pair != BEH(a)) { halt("deque_lookup: non-pair in chain"); }
        Pair p = (Pair)a;
        if (i <= 0) {
            return p->h;
        }
        --i;
        a = p->t;
    }
    return NOTHING;  // not found
}
inline void
deque_bind(Actor queue, Actor index, Actor item)
{
    int i;

    if (beh_integer != BEH(index)) { halt("deque_lookup: index must be an integer"); }
    i = ((Integer)index)->i;
    if (beh_deque != BEH(queue)) { halt("deque_bind: deque required"); }
    Pair q = (Pair)queue;
    Actor a = q->h;
    while (a != NIL) {
        if (beh_pair != BEH(a)) { halt("deque_bind: non-pair in chain"); }
        Pair p = (Pair)a;
        if (i <= 0) {
            p->h = item;
            break;
        }
        --i;
        a = p->t;
    }
    // not found
}

ACTOR the_empty_dict_actor = { expr_env_empty };
inline Actor
dict_new()
{
    return a_empty_dict;
}
inline Actor
dict_empty_p(Actor dict)
{
    return ((dict == a_empty_dict) ? a_true : a_false);
}
inline Actor
dict_lookup(Actor dict, Actor key)
{
    while (dict_empty_p(dict) == a_false) {
        if (expr_env != BEH(dict)) { halt("dict_lookup: non-dict in chain"); }
        Pair p = (Pair)dict;
        Actor a = p->h;
        if (beh_pair != BEH(a)) { halt("dict_lookup: non-pair entry"); }
        Pair q = (Pair)a;
        if (q->h == key) {
            return q->t;  // value
        }
        dict = p->t;  // next
    }
    return NOTHING;  // not found
}
inline Actor
dict_bind(Actor dict, Actor key, Actor value)
{
    Actor a = PR(PR(key, value), dict);
    BEH(a) = expr_env;  // override pair behavior with env behavior
    return a;
}

inline Actor
actor_new(Action beh)  // create an actor with only a behavior procedure
{
    Actor a = NEW(ACTOR);
    BEH(a) = beh;
    return a;
}
inline Actor
value_new(Action beh, Any data)  // create a "unserialized" (value) actor
{
    Value v = NEW(VALUE);
    CODE(v) = beh;
    DATA(v) = data;
    return (Actor)v;
}
inline Actor
serial_new(Action beh, Any data)  // create a "serialized" actor
{
    Serial s = NEW(SERIAL);
    BEH(s) = act_serial;
    VALUE(s) = value_new(beh, data);
    return (Actor)s;
}
inline Actor
serial_with_value(Actor v)  // create a "serialized" actor with "behavior" value
{
    Serial s = NEW(SERIAL);
    BEH(s) = act_serial;
    VALUE(s) = v;
    return (Actor)s;
}
inline void
actor_become(Actor s, Actor v)
{
    if (act_serial != BEH(s)) { halt("actor_become: serialized actor required"); }
    VALUE(s) = v;  // an "unserialzed" behavior actor
}

void
beh_event(Event e)
{
    TRACE(fprintf(stderr, "beh_event{event=%p}\n", e));
    expr_value(e);
}
inline Actor
event_new(Config cfg, Actor a, Actor m)
{
    // if (beh_config != BEH(cfg)) { halt("event_new: config actor required"); }
    Event e = NEW(EVENT);
    BEH(e) = beh_event;
    e->sponsor = cfg;
    e->target = a;
    e->message = m;
    return (Actor)e;
}

void
beh_host(Event e)
{
    beh_config(e);
}

void
beh_config(Event e)
{
    if (val_dispatch == BEH(MSG(e))) {
        Config guest = (Config)SELF(e);
        if (deque_empty_p(guest->events) != a_false) {
            TRACE(fprintf(stderr, "beh_config{msg=val_dispatch}: config=%p, <EMPTY>\n", guest));  
            // configuration execution is complete, do not schedule for dispatch
            return;
        }
        Actor event = deque_take(guest->events);
        TRACE(fprintf(stderr, "beh_config{msg=val_dispatch}: config=%p, guest=%p, event=%p\n", SPONSOR(e), guest, event));  
        config_enqueue(SPONSOR(e), event);
        config_send(SPONSOR(e), (Actor)guest, actor_new(val_dispatch)); // schedule guest configuration dispatch
    } else if (val_create_config == BEH(MSG(e))) {
        Actor cust = DATA(MSG(e));
        Config guest = config_new();
        TRACE(fprintf(stderr, "beh_config{msg=val_create_config}: config=%p, guest=%p, cust=%p\n", SPONSOR(e), guest, cust));
        config_send(SPONSOR(e), cust, (Actor)guest);
    } else {
        expr_value(e);
    }
}
inline Config
host_new()
{
    Config cfg = config_new();
    BEH(cfg) = beh_host;
    return cfg;
}
inline Config
config_new()
{
    Config cfg = NEW(CONFIG);
    BEH(cfg) = beh_config;
    cfg->events = deque_new();
    cfg->actors = list_new();
    return cfg;
}
inline void
config_enqueue(Config cfg, Actor e)
{
    if (beh_event != BEH(e)) { halt("config_enqueue: event actor required"); }
    deque_give(cfg->events, e);
}
inline void
config_enlist(Config cfg, Actor a)
{
    cfg->actors = list_push(cfg->actors, a);
}
inline void
config_send(Config cfg, Actor target, Actor msg)
{
    // if (beh_config != BEH(cfg)) { halt("config_send: config actor required"); }
    TRACE(fprintf(stderr, "config_send:     config=%p, actor=%p, msg=%p\n", cfg, target, msg));
    config_enqueue(cfg, event_new(cfg, target, msg));
}
Actor
config_dispatch(Config cfg)
{
    if (beh_host != BEH(cfg)) { halt("config_dispatch: host configuration actor required"); }
    if (deque_empty_p(cfg->events) != a_false) {
        TRACE(fprintf(stderr, "config_dispatch: config=%p, <EMPTY>\n", cfg));
        return NOTHING;
    }
    Actor a = deque_take(cfg->events);
    if (beh_event != BEH(a)) { halt("config_dispatch: event actor required"); }
    Event e = (Event)a;
    // event capability checks
    TRACE(fprintf(stderr, "config_dispatch: config=%p, actor=%p, msg=%p, event=%p\n", SPONSOR(e), SELF(e), MSG(e), e));
    (CODE(SELF(e)))(e);  // INVOKE ACTION PROCEDURE
    // TODO: reset watchdog timer
    return a;
}

void
act_serial(Event e)  // "serialized" actor behavior
{
    TRACE(fprintf(stderr, "act_serial{self=%p, msg=%p}\n", SELF(e), MSG(e)));
    (STRATEGY(SELF(e)))(e);  // INVOKE SERIALIZED BEHAVIOR
}

/**
LET pair_beh_0((ok, fail), t, tail) = \env'.[
    SEND ((ok, fail), #match, t, env') TO tail
]
**/
static void
beh_pair_0(Event e)
{
    TRACE(fprintf(stderr, "beh_pair_0{self=%p, msg=%p}\n", SELF(e), MSG(e)));
    Request r = (Request)DATA(SELF(e));
    ReqMatch rm = (ReqMatch)r->req;
    Actor tail = rm->env;  // extract hijacked target
    rm->env = (Actor)MSG(e);  // replace with extended environment
    config_send(SPONSOR(e), tail, (Actor)r);
}
/**
LET pair_beh(head, tail) = \msg.[
    LET ((ok, fail), req) = $msg IN
    CASE req OF
    (#eval, _) : [ SEND SELF TO ok ]
    (#match, $SELF, env) : [ SEND env TO ok ]
    (#match, (h, t), env) : [
        SEND ((ok', fail), #match, h, env) TO head
        CREATE ok' WITH \env'.[
            SEND ((ok, fail), #match, t, env') TO tail
        ]
    ]
    (#read) : [ SEND list_pop(SELF) TO ok ]
    (#write, value) : [ SEND list_push(SELF, value) TO ok ]
    _ : [ SEND (SELF, msg) TO fail ]
    END
]
**/
void
beh_pair(Event e)
{
    TRACE(fprintf(stderr, "beh_pair{self=%p, msg=%p}\n", SELF(e), MSG(e)));
    if (val_request != BEH(MSG(e))) { halt("beh_pair: request msg required"); }
    Request r = (Request)MSG(e);
    TRACE(fprintf(stderr, "beh_pair: ok=%p, fail=%p\n", r->ok, r->fail));
    if (val_req_eval == BEH(r->req)) {  // (#eval, _)
        TRACE(fprintf(stderr, "beh_pair: (#eval, _)\n"));
        config_send(SPONSOR(e), r->ok, SELF(e));
    } else if (val_req_match == BEH(r->req)) {  // (#match, value, env)
        ReqMatch rm = (ReqMatch)r->req;
        TRACE(fprintf(stderr, "beh_pair: (#match, %p, %p)\n", rm->value, rm->env));
        if (SELF(e) == rm->value) {
            config_send(SPONSOR(e), r->ok, rm->env);
        } else if (beh_pair == BEH(rm->value)) {
            Pair p = (Pair)SELF(e);
            Pair q = (Pair)rm->value;
            Actor ok = value_new(beh_pair_0, req_match_new(r->ok, r->fail, q->t, p->t));
            config_send(SPONSOR(e), p->h, req_match_new(ok, r->fail, q->h, rm->env));
        } else {
            TRACE(fprintf(stderr, "beh_pair: MISMATCH!\n"));
            config_send(SPONSOR(e), r->fail, (Actor)e);
        }
    } else if (val_req_read == BEH(r->req)) {  // (#read)
        Pair p = (Pair)SELF(e);
        TRACE(fprintf(stderr, "beh_pair: (#read) -> (%p, %p)\n", p->h, p->t));
        config_send(SPONSOR(e), r->ok, (Actor)list_pop(SELF(e)));
    } else if (val_req_write == BEH(r->req)) {  // (#write, value)
        ReqWrite rw = (ReqWrite)r->req;
        TRACE(fprintf(stderr, "beh_pair: (#write, %p)\n", rw->value));
        config_send(SPONSOR(e), r->ok, list_push(SELF(e), rw->value));
    } else {
        TRACE(fprintf(stderr, "beh_pair: FAIL!\n"));
        config_send(SPONSOR(e), r->fail, (Actor)e);
    }
}

/**
LET deque_beh(head, tail) = \msg.[
    LET ((ok, fail), req) = $msg IN
    CASE req OF
    (#eval, _) : [ SEND SELF TO ok ]
    (#match, $SELF, env) : [ SEND env TO ok ]
    (#read) : [ SEND (deque_take(SELF), SELF) TO ok ]
    (#write, value) : [
        LET _ = $deque_give(SELF, value)
        IN SEND SELF TO ok
    ]
    _ : [ SEND (SELF, msg) TO fail ]
    END
]
**/
void
beh_deque(Event e)
{
    TRACE(fprintf(stderr, "beh_deque{self=%p, msg=%p}\n", SELF(e), MSG(e)));
    if (val_request != BEH(MSG(e))) { halt("beh_deque: request msg required"); }
    Request r = (Request)MSG(e);
    TRACE(fprintf(stderr, "beh_deque: ok=%p, fail=%p\n", r->ok, r->fail));
    if (val_req_eval == BEH(r->req)) {  // (#eval, _)
        TRACE(fprintf(stderr, "beh_deque: (#eval, _)\n"));
        config_send(SPONSOR(e), r->ok, SELF(e));
    } else if (val_req_match == BEH(r->req)) {  // (#match, value, env)
        ReqMatch rm = (ReqMatch)r->req;
        TRACE(fprintf(stderr, "beh_deque: (#match, %p, %p)\n", rm->value, rm->env));
        if (SELF(e) == rm->value) {
            config_send(SPONSOR(e), r->ok, rm->env);
        } else {
            TRACE(fprintf(stderr, "beh_deque: MISMATCH!\n"));
            config_send(SPONSOR(e), r->fail, (Actor)e);
        }
    } else if (val_req_read == BEH(r->req)) {  // (#read)
        Pair p = (Pair)SELF(e);
        TRACE(fprintf(stderr, "beh_deque: (#read) -> (%p, %p)\n", p->h, p->t));
        config_send(SPONSOR(e), r->ok, PR(deque_take(SELF(e)), SELF(e)));
    } else if (val_req_write == BEH(r->req)) {  // (#write, value)
        ReqWrite rw = (ReqWrite)r->req;
        TRACE(fprintf(stderr, "beh_deque: (#write, %p)\n", rw->value));
        deque_give(SELF(e), rw->value);
        config_send(SPONSOR(e), r->ok, SELF(e));
    } else {
        TRACE(fprintf(stderr, "beh_deque: FAIL!\n"));
        config_send(SPONSOR(e), r->fail, (Actor)e);
    }
}

/**
LET oper_true_beh = \msg.[
    LET ((ok, fail), req) = $msg IN
	CASE req OF
	(#comb, (expr, _), env) : [
        SEND ((ok, fail), #eval, env) TO expr
    ]
	_ : value_beh(msg)  # delegate
	END
]
**/
static void
comb_true(Event e)
{
    TRACE(fprintf(stderr, "comb_true{self=%p, msg=%p}\n", SELF(e), MSG(e)));
    if (val_request != BEH(MSG(e))) { halt("comb_true: request msg required"); }
    Request r = (Request)MSG(e);
    TRACE(fprintf(stderr, "comb_true: ok=%p, fail=%p\n", r->ok, r->fail));
    if (val_req_combine == BEH(r->req)) {  // (#comb, (expr, _), env)
        ReqCombine rc = (ReqCombine)r->req;
        TRACE(fprintf(stderr, "comb_true: (#comb, %p, %p)\n", rc->opnd, rc->env));
        if (beh_pair == BEH(rc->opnd)) {
            Pair p = (Pair)rc->opnd;
            Actor expr = p->h;
            config_send(SPONSOR(e), expr, req_eval_new(r->ok, r->fail, rc->env));
        } else {
            TRACE(fprintf(stderr, "comb_true: opnd must be a Pair\n"));
            config_send(SPONSOR(e), r->fail, (Actor)e);
        }
    } else {
        expr_value(e);  // delegate
    }
}
/**
CREATE true_oper WITH oper_true_beh
**/
ACTOR the_true_actor = { comb_true };

/**
LET oper_false_beh = \msg.[
    LET ((ok, fail), req) = $msg IN
	CASE req OF
	(#comb, (_, expr), env) : [
        SEND ((ok, fail), #eval, env) TO expr
    ]
	_ : value_beh(msg)  # delegate
	END
]
**/
static void
comb_false(Event e)
{
    TRACE(fprintf(stderr, "comb_false{self=%p, msg=%p}\n", SELF(e), MSG(e)));
    if (val_request != BEH(MSG(e))) { halt("comb_false: request msg required"); }
    Request r = (Request)MSG(e);
    TRACE(fprintf(stderr, "comb_false: ok=%p, fail=%p\n", r->ok, r->fail));
    if (val_req_combine == BEH(r->req)) {  // (#comb, (_, expr), env)
        ReqCombine rc = (ReqCombine)r->req;
        TRACE(fprintf(stderr, "comb_false: (#comb, %p, %p)\n", rc->opnd, rc->env));
        if (beh_pair == BEH(rc->opnd)) {
            Pair p = (Pair)rc->opnd;
            Actor expr = p->t;
            config_send(SPONSOR(e), expr, req_eval_new(r->ok, r->fail, rc->env));
        } else {
            TRACE(fprintf(stderr, "comb_false: opnd must be a Pair\n"));
            config_send(SPONSOR(e), r->fail, (Actor)e);
        }
    } else {
        expr_value(e);  // delegate
    }
}
/**
CREATE false_oper WITH oper_false_beh
**/
ACTOR the_false_actor = { comb_false };

static void
beh_ignore(Event e)
{
    TRACE(fprintf(stderr, "beh_ignore{self=%p, msg=%p}\n", SELF(e), MSG(e)));
}
ACTOR the_ignore_actor = { beh_ignore };

void
beh_halt(Event e)
{
    TRACE(fprintf(stderr, "beh_halt{event=%p}\n", e));
    halt("HALT!");
}
VALUE the_halt_actor = { { beh_halt }, NOTHING };  // qualifies as both VALUE and SERIAL

void
val_create_config(Event e)
{
    expr_value(e);
}

void
val_dispatch(Event e)
{
    expr_value(e);
}