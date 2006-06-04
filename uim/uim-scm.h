/*

  Copyright (c) 2003-2006 uim Project http://uim.freedesktop.org/

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of authors nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

/*                         !!CAUTION!!

   This API is designed for input method plugin and internal uim
   implementation. Other developers should not use this API since the
   API easily causes fatal crash involving GC if you does not pay
   attention enough. Be careful.

   This API is not intended to provide all R5RS features. Only 'core'
   ones to write Scheme-C adapters should be added. Consider how
   frequently it will be used, and whether it should be written by C,
   when you add an API function.

                                                2005-01-10 YamaKen
*/


/* API and ABI are unstable */
#ifndef _uim_scm_h_included_
#define _uim_scm_h_included_

#include <stdio.h>
#include "uim.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  UIM_SCM_EXTENDED_API indicates that the interfaces are extended in
  the r5rs barnch. Some of interfaces are re-adopted from
  uim-compat-scm.h. Since it contradicts with the "core interface
  policy" described above (2005-01-10 YamaKen), it must be
  distinguished from original ones for now. A discussion may be
  required to make the new API standard before next stable series
  has been released.  -- YamaKen 2005-07-29
*/
#define UIM_SCM_EXTENDED_API

#define UIM_SCM_GCC4_READY_GC 1

/*
  uim companion tools should treat lisp object as opaque. struct
  uim_opaque exists only for type check and has no actual definition.
*/
typedef struct uim_opaque * uim_lisp;
typedef void (*uim_func_ptr)(void);

#define UIM_SCM_FALSEP(x)  (uim_scm_eq((x), uim_scm_f()))
#define UIM_SCM_NFALSEP(x) (!uim_scm_eq((x), uim_scm_f()))

#if 1
/* deprecated: replace with UIM_SCM_N?FALSEP() */
#define FALSEP(x)  (UIM_SCM_FALSEP(x))
#define NFALSEP(x) (UIM_SCM_NFALSEP(x))
#define TRUEP(x) (uim_scm_eq(x, uim_scm_t()))
#define NTRUEP(x) (!uim_scm_eq(x, uim_scm_t()))
#endif


#if UIM_SCM_GCC4_READY_GC
/*
 * The variable to ensure that a call of uim_scm_gc_protect_stack() is
 * uninlined in portable way through (*f)().
 *
 * Don't access this variables directly. Use
 * UIM_SCM_GC_PROTECTED_CALL*() instead.
 */
extern uim_lisp *(*volatile uim_scm_gc_current_stack_ptr)(void);
extern uim_lisp *(*volatile uim_scm_gc_protect_stack_ptr)(uim_lisp *);
#endif /* UIM_SCM_GCC4_READY_GC */


/* 'uim_scm' prefix is not appropriate for these functions... any ideas? */
FILE *
uim_scm_get_output(void);
void
uim_scm_set_output(FILE *fp);
  
uim_bool
uim_scm_is_alive(void);
long
uim_scm_get_verbose_level(void);
void
uim_scm_set_verbose_level(long new_value);
void
uim_scm_set_lib_path(const char *path);

#if UIM_SCM_GCC4_READY_GC
#ifdef __GNUC__
#define UIM_SCM_NOINLINE __attribute__((__noinline__))
#else
#define UIM_SCM_NOINLINE
#endif /* __GNUC__ */

/*
 * Function caller with protecting lisp objects on stack from GC
 *
 * The protection is safe against with variable reordering on a stack
 * frame performed in some compilers as anti-stack smashing or
 * optimization.
 */
#define UIM_SCM_GC_PROTECTED_CALL(ret, ret_type, func, args)                 \
    UIM_SCM_GC_PROTECTED_CALL_INTERNAL(ret = , ret_type, func, args)

#define UIM_SCM_GC_PROTECTED_CALL_VOID(func, args)                           \
    UIM_SCM_GC_PROTECTED_CALL_INTERNAL((void), void, func, args)

#define UIM_SCM_GC_PROTECTED_CALL_INTERNAL(exp_ret, ret_type, func, args)    \
    do {                                                                     \
        /* ensure that func is uninlined */                                  \
        ret_type (*volatile fp)() = (ret_type (*)())&func;                   \
        uim_lisp *stack_start;                                               \
                                                                             \
        if (0) exp_ret func args;  /* compile-time type check */             \
        stack_start = uim_scm_gc_current_stack();                            \
        uim_scm_gc_protect_stack(stack_start);                               \
        exp_ret (*fp)args;                                                   \
        uim_scm_gc_unprotect_stack(stack_start);                             \
    } while (/* CONSTCOND */ 0)

/*
 * Ordinary programs should not call these functions directly. Use
 * UIM_SCM_GC_PROTECTED_CALL*() instead.
 */
#ifdef __GNUC__
#define uim_scm_gc_current_stack uim_scm_gc_current_stack_internal
#define uim_scm_gc_protect_stack uim_scm_gc_protect_stack_internal
#else /* __GNUC__ */
#define uim_scm_gc_current_stack (*uim_scm_gc_current_stack_ptr)
#define uim_scm_gc_protect_stack (*uim_scm_gc_protect_stack_ptr)
#endif /* __GNUC__ */
/*
 * The location must be initialized with a valid uim_lisp object
 * before calling uim_scm_gc_protect().
 */
void
uim_scm_gc_protect(uim_lisp *location);
void
uim_scm_gc_unprotect_stack(uim_lisp *stack_start);

uim_lisp *
uim_scm_gc_current_stack_internal(void) UIM_SCM_NOINLINE;
uim_lisp *
uim_scm_gc_protect_stack_internal(uim_lisp *stack_start) UIM_SCM_NOINLINE;
#else /* UIM_SCM_GCC4_READY_GC */
void
uim_scm_gc_protect(uim_lisp *location);
void
uim_scm_gc_protect_stack(uim_lisp *stack_start);
void
uim_scm_gc_unprotect_stack(uim_lisp *stack_start);
#endif /* UIM_SCM_GCC4_READY_GC */

/* evaluations */
uim_bool
uim_scm_load_file(const char *fn);
uim_bool
uim_scm_require_file(const char *fn);
uim_lisp
uim_scm_eval(uim_lisp obj);
uim_lisp
uim_scm_eval_c_string(const char *str);
uim_lisp
uim_scm_return_value(void);
/*
  TODO: reorganize UIM_EVAL_FSTRINGn(), uim_sizeof_sexp_str() and
  uim_eval_string() in uim-internal.h into this file
*/

/* type conversions */
uim_bool
uim_scm_c_bool(uim_lisp val);
uim_lisp
uim_scm_make_bool(uim_bool val);

int
uim_scm_c_int(uim_lisp integer);
uim_lisp
uim_scm_make_int(int integer);

char *
uim_scm_c_str(uim_lisp str);
const char *
uim_scm_refer_c_str(uim_lisp str);
uim_lisp
uim_scm_make_str(const char *str);

char *
uim_scm_c_symbol(uim_lisp str);
uim_lisp
uim_scm_make_symbol(const char *str);

void *
uim_scm_c_ptr(uim_lisp ptr);
uim_lisp
uim_scm_make_ptr(void *ptr);

uim_func_ptr
uim_scm_c_func_ptr(uim_lisp func_ptr);
uim_lisp
uim_scm_make_func_ptr(uim_func_ptr func_ptr);

void
uim_scm_init_subr_0(const char *name, uim_lisp (*fcn)(void));
void 
uim_scm_init_subr_1(const char *name, uim_lisp (*fcn)(uim_lisp));
void
uim_scm_init_subr_2(const char *name, uim_lisp (*fcn)(uim_lisp, uim_lisp));
void
uim_scm_init_subr_3(const char *name, uim_lisp (*fcn)(uim_lisp, uim_lisp, uim_lisp));
void
uim_scm_init_subr_4(const char *name, uim_lisp (*fcn)(uim_lisp, uim_lisp, uim_lisp,
		   				 uim_lisp));
void
uim_scm_init_subr_5(const char *name, uim_lisp (*fcn)(uim_lisp, uim_lisp, uim_lisp,
						uim_lisp, uim_lisp));

/* constants */
uim_lisp
uim_scm_t(void);
uim_lisp
uim_scm_f(void);
uim_lisp
uim_scm_null_list(void);

/* predicates */
uim_bool
uim_scm_nullp(uim_lisp obj);
uim_bool
uim_scm_consp(uim_lisp obj);
uim_bool
uim_scm_integerp(uim_lisp obj);
uim_bool
uim_scm_stringp(uim_lisp obj);
uim_bool
uim_scm_eq(uim_lisp a, uim_lisp b);
uim_bool
uim_scm_string_equal(uim_lisp a, uim_lisp b);

/* list operations */
uim_lisp
uim_scm_cons(uim_lisp car, uim_lisp cdr);
uim_lisp
uim_scm_car(uim_lisp pair);
uim_lisp
uim_scm_cdr(uim_lisp pair);
uim_lisp
uim_scm_cadr(uim_lisp lst);
uim_lisp
uim_scm_caar(uim_lisp lst);
uim_lisp
uim_scm_cdar(uim_lisp lst);
uim_lisp
uim_scm_cddr(uim_lisp lst);
uim_lisp
uim_scm_length(uim_lisp lst);
uim_lisp
uim_scm_reverse(uim_lisp lst);


#ifdef __cplusplus
}
#endif
#endif
