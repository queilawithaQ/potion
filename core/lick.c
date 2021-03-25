///\file lick.c
///\class PNLick - the interleaved data format, a named tree node
///\class PNCons - node of a chained list (head, tail)
//
// (c) 2008 why the lucky stiff, the freelance professor
// (c) 2014 perl11 org
//
#include <stdio.h>
#include <stdlib.h>
#include "potion.h"
#include "internal.h"

PN potion_lick(Potion *P, PN name, PN inner, PN attr) {
  vPN(Lick) lk = PN_ALLOC(PN_TLICK, struct PNLick);
  lk->name = name;
  lk->attr = attr;
  lk->inner = inner;
  return (PN)lk;
}

///\memberof PNLick
/// "attr" method
///\return the attached attr member PN
PN potion_lick_attr(Potion *P, PN cl, PN self) {
  return ((struct PNLick *)self)->attr;
}

///\memberof PNLick
/// "licks" method. attached can be a string or PNTuple
///\return the attached licks PNTuple or PN_NIL
PN potion_lick_licks(Potion *P, PN cl, PN self) {
  PN licks = ((struct PNLick *)self)->inner;
  if (PN_IS_TUPLE(licks)) return licks;
  return PN_NIL;
}

///\memberof PNLick
/// "name" method
///\return PNString
PN potion_lick_name(Potion *P, PN cl, PN self) {
  return ((struct PNLick *)self)->name;
}

///\memberof PNLick
/// "text" method. attached can be a string or PNTuple
///\return the attached text PNString or PN_NIL
PN potion_lick_text(Potion *P, PN cl, PN self) {
  PN text = ((struct PNLick *)self)->inner;
  if (PN_IS_STR(text)) return text;
  return PN_NIL;
}

///\memberof PNLick
/// "string" method
///\return space seperated PNString of the lick members: name inner attr
PN potion_lick_string(Potion *P, PN cl, PN self) {
  PN out = potion_byte_str(P, "");
  potion_bytes_obj_string(P, out, ((struct PNLick *)self)->name);
  if (((struct PNLick *)self)->inner != PN_NIL) {
    pn_printf(P, out, " ");
    potion_bytes_obj_string(P, out, ((struct PNLick *)self)->inner);
  }
  if (((struct PNLick *)self)->attr != PN_NIL) {
    pn_printf(P, out, " ");
    potion_bytes_obj_string(P, out, ((struct PNLick *)self)->attr);
  }
  return PN_STR_B(out);
}

// internal cons. add tail to head. tail usually nil.
struct PNCons * potion_cons(Potion *P, PN head, PN tail) {
  vPN(Cons) cons = PN_ALLOC(PN_TCONS, struct PNCons);
  cons->head = head;
  cons->tail = tail;
  return cons;
}

///\memberof Lobby
/// global "cons" method. allocate a new cons
///\return PNCons (head . nil)
PN potion_lobby_cons(Potion *P, PN cl, PN self, PN head) {
  vPN(Cons) cons = PN_ALLOC(PN_TCONS, struct PNCons);
  cons->head = head;
  cons->tail = PN_NIL;
  return (PN)cons;
}

///\memberof PNCons
/// push a value to a cons
///\return PNCons
PN potion_list_cons(Potion *P, PN cl, PN self, PN value) {
  vPN(Cons) cons = potion_cons(P, value, self);
  return (_PN)cons;
}

///\memberof PNCons
///\return PNCons
PN potion_list_head(Potion *P, PN cl, PN self) {
  return ((struct PNCons*)self)->head;
}

///\memberof PNCons
///\return PNCons
PN potion_list_tail(Potion *P, PN cl, PN self) {
  return ((struct PNCons*)self)->tail;
}

///\memberof PNCons
///\return PNCons
PN potion_list_sethead(Potion *P, PN cl, PN self, PN head) {
  ((struct PNCons*)self)->head = head;
  return self;
}

///\memberof PNCons
///\return PNCons
PN potion_list_settail(Potion *P, PN cl, PN self, PN tail) {
  ((struct PNCons*)self)->tail = tail;
  return self;
}

///\memberof PNCons
///\return PNCons
PN potion_list_nth(Potion *P, PN cl, PN self, PN index) {
  int i;
  vPN(Cons) tmp = (struct PNCons*)self;
  for (i=0; tmp && i < PN_NUM(index); i++, tmp = (struct PNCons*)tmp->tail);
  return (PN)tmp;
}

///\memberof PNCons
///\return PNCons
PN potion_list_setnth(Potion *P, PN cl, PN self, PN index, PN value) {
  PN tmp = potion_list_nth(P, cl, self, index);
  tmp = value;
  return (PN)tmp;
}

///\memberof PNCons
///\return PNCons
PN potion_list_append(Potion *P, PN cl, PN self, PN list) {
  vPN(Cons) tmp = (struct PNCons*)self;
  while (tmp->tail) { tmp = (struct PNCons*)tmp->tail; }
  tmp->tail = list;
  return self;
}

///\memberof PNCons
///\return PNCons
PN potion_list_member(Potion *P, PN cl, PN self, PN value) {
  do {
    // which equality?
    if (((struct PNCons*)self)->head == value)
      return self;
  } while (((struct PNCons*)self)->tail);
  return PN_NIL;
}

PN potion_list_reverse_r(Potion *P, struct PNCons* head, PN tail) {
  if (head)
    return potion_list_reverse_r(P, (struct PNCons*)head->tail,
               (PN)potion_cons(P, head->head, tail));
  else
    return PN_NIL;
}
///\memberof PNCons
///\return PNCons
PN potion_list_reverse(Potion *P, PN cl, PN self) {
  //(if l (reverse (cdr l) (cons (car l) r))
  vPN(Cons) head = (vPN(Cons))self;
  if (head)
    return potion_list_reverse_r(P, (struct PNCons*)head->tail,
               (PN)potion_cons(P, head->head, PN_NIL));
  return self;
}

///\memberof PNCons
///\return PNCons
PN potion_list_nreverse(Potion *P, PN cl, PN self) {
  vPN(Cons) list = (vPN(Cons))self;
  vPN(Cons) head = NULL;
  vPN(Cons) next;
  for (; list; list = next) {
    next = list->tail;
    list->tail = head;
    head = list;
  }
  return (PN)head;
}

void potion_lick_init(Potion *P) {
  PN c_vt = PN_VTABLE(PN_TCONS); // should inherit from nil
  potion_method(P->lobby, "cons", potion_lobby_cons, "head=o");
  potion_method(c_vt, "cons", potion_list_cons, "list=C");
  potion_method(c_vt, "head", potion_list_head, 0);
  potion_method(c_vt, "tail", potion_list_tail, 0);
  potion_method(c_vt, "sethead", potion_list_sethead, "value=o");
  potion_method(c_vt, "settail", potion_list_settail, "list=o");
  potion_type_call_is(c_vt, PN_FUNC(potion_list_nth, "index=N"));
  potion_type_callset_is(c_vt, PN_FUNC(potion_list_setnth, "index=N,value=o"));
  potion_method(c_vt, "append", potion_list_append, "list=o");
  potion_method(c_vt, "member", potion_list_member, "value=o");
  potion_method(c_vt, "reverse", potion_list_reverse, 0);
  potion_method(c_vt, "nreverse", potion_list_nreverse, 0);
  PN l_vt = PN_VTABLE(PN_TLICK);
  potion_method(l_vt, "attr", potion_lick_attr, 0);
  potion_method(l_vt, "licks", potion_lick_licks, 0);
  potion_method(l_vt, "name", potion_lick_name, 0);
  potion_method(l_vt, "string", potion_lick_string, 0);
  potion_method(l_vt, "text", potion_lick_text, 0);
}
