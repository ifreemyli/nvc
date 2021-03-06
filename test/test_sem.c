#include "type.h"
#include "phase.h"
#include "util.h"
#include "common.h"

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct error {
   int        line;
   const char *snippet;
} error_t;

static const error_t  *error_lines = NULL;
static error_fn_t orig_error_fn = NULL;

static void setup(void)
{
   const char *lib_dir = getenv("LIB_DIR");
   if (lib_dir)
      lib_add_search_path(lib_dir);

   lib_set_work(lib_tmp());
   opt_set_int("bootstrap", 0);
   opt_set_int("unit-test", 1);
   opt_set_int("relax", 0);
}

static void teardown(void)
{
   lib_free(lib_work());
}

static void test_error_fn(const char *msg, const loc_t *loc)
{
   fail_if(error_lines == NULL);

   bool unexpected = error_lines->line == -1
      || error_lines->snippet == NULL
      || error_lines->line != loc->first_line
      || strstr(msg, error_lines->snippet) == NULL;

   if (unexpected) {
      orig_error_fn(msg, loc);
      printf("expected line %d '%s'\n",
             error_lines->line, error_lines->snippet);
   }

   fail_if(unexpected);

   error_lines++;
}

static void expect_errors(const error_t *lines)
{
   fail_unless(orig_error_fn == NULL);
   orig_error_fn = set_error_fn(test_error_fn);
   error_lines = lines;
}

START_TEST(test_integer)
{
   tree_t a, d, p, s, e;
   type_t t;
   range_t r;

   input_from_file(TESTDIR "/sem/integer.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   const error_t expect[] = {
      { 20, "MY_INT1 does not match type of target MY_INT2" },
      { 30, "MY_INT1 does not match type of target MY_INT2_SUB" },
      { 35, "type NOTHING is not declared" },
      { 48, "no suitable overload for operator \"*\" [MY_INT2, MY_INT1" },
      { 57, "MY_INT2 has no attribute CAKE" },
      { -1, NULL }
   };
   expect_errors(expect);

   sem_check(a);
   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);

   d = tree_decl(a, 2);
   fail_unless(tree_ident(d) == ident_new("X"));
   t = tree_type(d);
   fail_unless(type_kind(t) == T_INTEGER);
   e = tree_value(d);
   fail_unless(tree_kind(e) == T_LITERAL);
   t = tree_type(e);
   fail_unless(type_kind(t) == T_INTEGER);

   fail_unless(tree_stmts(a) == 6);

   // Process 1

   p = tree_stmt(a, 0);
   fail_unless(tree_kind(p) == T_PROCESS);

   fail_unless(tree_decls(p) == 1);

   d = tree_decl(p, 0);
   fail_unless(tree_ident(d) == ident_new("Z"));
   fail_unless(type_kind(tree_type(d)) == T_INTEGER);

   s = tree_stmt(p, 0);
   fail_unless(tree_ref(tree_target(s)) == d);

   // Process 6

   p = tree_stmt(a, 5);
   fail_unless(tree_kind(p) == T_PROCESS);
   fail_unless(tree_decls(p) == 1);

   d = tree_decl(p, 0);
   r = type_dim(tree_type(d), 0);

   s = tree_stmt(p, 0);
   fail_unless(tree_kind(tree_value(s)) == T_ATTR_REF);
   fail_unless(tree_value(tree_value(s)) == r.left);
   s = tree_stmt(p, 1);
   fail_unless(tree_kind(tree_value(s)) == T_ATTR_REF);
   fail_unless(tree_value(tree_value(s)) == r.right);
   s = tree_stmt(p, 2);
   fail_unless(tree_kind(tree_value(s)) == T_ATTR_REF);
   fail_unless(tree_value(tree_value(s)) == r.right);

}
END_TEST

START_TEST(test_ports)
{
   tree_t a, e, p;

   input_from_file(TESTDIR "/sem/ports.vhd");

   const error_t expect[] = {
      { 31,  "cannot read output port O" },
      { 42,  "cannot assign to input port I" },
      { 81,  "missing actual for formal I" },
      { 85,  "formal I already has an actual" },
      { 89,  "too many positional actuals" },
      { 92,  "no visible declaration for CAKE" },
      { 94,  "cannot find unit WORK.BAD" },
      { 103, "unconnected port I with mode IN must have a default value" },
      { 116, "object X is not a component declaration" },
      { 148, "actual must be globally static expression" },
      { 155, "no visible declaration for Q" },
      { 163, "no visible declaration for U" },
      { 168, "formal name must be static" },
      { 177, "formal name must be static" },
      { 185, "no visible declaration for HELLO" },
      { 217, "port O of unconstrained type INT_VEC cannot be unconnected" },
      { 221, "type of actual universal real does not match type INTEGER" },
      { 271, "result of conversion for unconstrained formal I must" },
      { 279, "actual must be globally static expression or locally" },
      { 283, "conversion not allowed for formal O with mode OUT" },
      { 293, "output conversion not allowed for formal I with mode IN" },
      { 297, "output conversion for formal B must not have OPEN actual" },
      { 311, "cannot assign to input port X" },
      { 312, "cannot read output port Y" },
      { 332, "cannot read parameter X with mode IN" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   // Entity foo

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Entity top

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   // Architecture test

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Architecture other

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Architecture conv

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Entity ent_with_vec

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   // Architecture test

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Architecture other2

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST


START_TEST(test_scope)
{
   tree_t a, e, p;

   input_from_file(TESTDIR "/sem/scope.vhd");

   const error_t expect[] = {
      {  31, "WORK.PACK1.MY_INT1 does not match type"
         " of target WORK.PACK2.MY_INT1" },
      {  44, "WORK.PACK1.MY_INT1 does not match type of target MY_INT1" },
      {  63, "G already declared in this region" },
      {  71, "P already declared in this region" },
      { 114, "type MY_INT1 is not declared" },
      { 137, "no visible declaration for E1" },
      { 160, "no visible declaration for FUNC2" },
      { 167, "declaration NOT_HERE not found in unit WORK.PACK5" },
      { 189, "type MY_INT1 is not declared" },
      { 236, "missing library clause for FOO" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   for (int i = 0; i < 3; i++) {
      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   for (int i = 0; i < 2; i++) {
      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   for (int i = 0; i < 2; i++) {
      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   for (int i = 0; i < 2; i++) {
      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   for (int i = 0; i < 2; i++) {
      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_ambiguous)
{
   tree_t a, e, p, s;
   type_t lhs, rhs;

   input_from_file(TESTDIR "/sem/ambiguous.vhd");

   const error_t expect[] = {
      { 35,  "type of value BAR does not match type of target FOO" },
      { 56,  "type of aggregate is ambiguous" },
      { 86,  "ambiguous use of enumeration literal FALSE" },
      { 93,  "ambiguous call to function NOW" },
      { 103, "ambiguous use of name FALSE" },
      { 141, "ambiguous use of operator \"<\"" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   p = tree_stmt(a, 0);
   fail_unless(tree_stmts(p) == 2);
   s = tree_stmt(p, 0);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(tree_waveform(s, 0)));
   fail_unless(type_ident(lhs) == ident_new("FOO"));
   fail_unless(type_ident(rhs) == ident_new("FOO"));
   s = tree_stmt(p, 1);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(tree_waveform(s, 0)));
   fail_unless(type_ident(lhs) == ident_new("BAR"));
   fail_unless(type_ident(rhs) == ident_new("BAR"));

   p = tree_stmt(a, 1);
   fail_unless(tree_stmts(p) == 2);
   s = tree_stmt(p, 0);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(tree_waveform(s, 0)));
   fail_unless(type_ident(lhs) == ident_new("FOO"));
   fail_unless(type_ident(rhs) == ident_new("FOO"));
   s = tree_stmt(p, 1);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(tree_waveform(s, 0)));
   fail_unless(type_ident(lhs) == ident_new("BAR"));
   fail_unless(type_ident(rhs) == ident_new("BAR"));

   p = tree_stmt(a, 2);
   fail_unless(tree_stmts(p) == 3);
   s = tree_stmt(p, 0);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(s));
   fail_unless(type_ident(lhs) == ident_new("BAZ"));
   fail_unless(type_ident(rhs) == ident_new("BAZ"));
   s = tree_stmt(p, 1);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(s));
   fail_unless(type_ident(lhs) == ident_new("BAZ"));
   fail_unless(type_ident(rhs) == ident_new("BAZ"));
   s = tree_stmt(p, 2);
   lhs = tree_type(tree_target(s));
   rhs = tree_type(tree_value(tree_waveform(s, 0)));
   fail_unless(type_ident(lhs) == ident_new("FOO"));
   fail_unless(type_ident(rhs) == ident_new("FOO"));

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   for (int i = 0; i < 5; i++) {
      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_const)
{
   tree_t a, e, p;

   input_from_file(TESTDIR "/sem/const.vhd");

   const error_t expect[] = {
      { 19, "invalid target of variable assignment" },
      { 23, "deferred constant declarations are only permitted" },
      { 53, "constant WORK.P.C already has a value" },
      { 54, "expected type INTEGER for deferred constant WORK.P.F" },
      { 44, "deferred constant WORK.P.D was not given a value" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_std)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/std.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);
}
END_TEST

START_TEST(test_wait)
{
   tree_t a, e, p, w;

   input_from_file(TESTDIR "/sem/wait.vhd");

   const error_t expect[] = {
      { 17, "type of delay must be TIME" },
      { 26, "name V in sensitivity list is not a signal" },
      { 35, "invalid use of A" },
      { 40, "wait statement not allowed in process" },
      { 51, "type of condition must be BOOLEAN" },
      { 53, "type of delay must be TIME" },
      { 66, "name in sensitivity list is not static" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   p = tree_stmt(a, 0);
   fail_unless(tree_kind(p) == T_PROCESS);
   fail_unless(tree_stmts(p) == 4);

   ident_t time = ident_new("STD.STANDARD.TIME");

   w = tree_stmt(p, 0);
   fail_unless(tree_kind(w) == T_WAIT);
   fail_unless(type_ident(tree_type(tree_delay(w))) == time);

   w = tree_stmt(p, 1);
   fail_unless(tree_kind(w) == T_WAIT);
   fail_unless(type_ident(tree_type(tree_delay(w))) == time);

   w = tree_stmt(p, 2);
   fail_unless(tree_kind(w) == T_WAIT);
   fail_unless(type_ident(tree_type(tree_delay(w))) == time);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_func)
{
   tree_t p;

   input_from_file(TESTDIR "/sem/func.vhd");

   const error_t expect[] = {
      {   5, "function arguments must have mode IN" },
      {  17, "must be an unconstrained array type" },
      {  21, "resolution function must have single argument" },
      {  25, "declaration UENUM is not a function" },
      {  27, "type of default value universal integer does not" },
      {  29, "subprogram body is not allowed in package specification" },
      {  36, "unit WORK.BAD not found in library WORK" },
      {  48, "name A cannot be used in this context" },
      {  51, "function arguments must have mode IN" },
      {  56, "function must contain a return statement" },
      {  62, "duplicate declaration of function FOO" },
      { 114, "positional parameters must precede named parameters" },
      { 115, "duplicate parameter name X" },
      { 124, "function arguments may not have VARIABLE class" },
      { 146, "object X has class CONSTANT and cannot be associated" },
      { 161, "pure function WORK.FUNC.TEST18 cannot call impure function" },
      { 166, "object X with access type must have class VARIABLE" },
      { 180, "no suitable overload for function TEST20" },
      { 181, "missing actual for formal Y without default value" },
      { 182, "no suitable overload for function TEST20" },
      { 239, "class variable of subprogram body WORK.FUNC2.TEST25 paramteter" },
      { 245, "class constant of subprogram body WORK.FUNC2.TEST26 paramteter" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_array)
{
   tree_t p, a, e;

   input_from_file(TESTDIR "/sem/array.vhd");

   const error_t expect[] = {
      { 27, "positional associations must appear first in aggregate" },
      { 33, "named association must not follow others" },
      { 39, "only a single others association allowed" },
      { 46, "type of initial value universal integer does not match" },
      { 55, "type of value universal integer does not match type of" },
      { 57, "type of value INT_ARRAY does not match type" },
      { 65, "for operator \"=\" [INT_ARRAY, TEN_INTS return BOOLEAN]" },
      { 88, "array W has 2 dimensions but 1 indices given" },
      { 89, "array W has 2 dimensions but 3 indices given" },
      { 98, "type of index universal integer does not match type" },
      { 102, "named and positional associations cannot be mixed in" },
      { 111, "non-locally static choice must be only choice" },
      { 119, "type of slice prefix is not an array" },
      { 120, "range direction of slice does not match prefix" },
      { 121, "others choice not allowed in this context" },
      { 130, "range direction of slice does not match prefix" },
      { 207, "array BAD cannot have unconstrained element type" },
      { 215, "others choice not allowed in this context" },
      { 232, "aliased name is not static" },
      { 233, "aliased name is not static" },
      { 234, "type of aliased object INT_ARRAY does not match" },
      { 241, "no visible declaration for I" },
      { 246, "universal integer bound must be numeric literal or attribute" },
      { 252, "expected 1 constraints for type INT_ARRAY but found 2" },
      { 272, "no suitable overload for operator \"<\" [INT2D, INT2D" },
      { 277, "type NOT_HERE is not declared" },
      { 279, "type NUM_ARRAY is unconstrained" },
      { 285, "object K does not have a range" },
      { 295, "type of index universal integer does not match" },
      { 343, "invalid character 'f' in string literal of type BIT_VECTOR" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_assert)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/assert.vhd");

   const error_t expect[] = {
      { 17, "type of assertion expression must be BOOLEAN" },
      { 22, "type of message be STRING" },
      { 27, "type of severity must be SEVERITY_LEVEL" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_generics)
{
   tree_t a, e, p;

   input_from_file(TESTDIR "/sem/generics.vhd");

   const error_t expect[] = {
      {  34, "missing actual for formal N" },
      {  38, "too many positional actuals" },
      {  48, "no visible declaration for X" },
      {  58, "invalid object class for generic" },
      {  68, "no visible declaration for Y" },
      { 109, "actual must be globally static expression or locally " },
      { -1, NULL }
   };
   expect_errors(expect);

   // Entity bot

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Entity top

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   // Entity bad

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   // Entity class

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   // Package p

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   // Entity static

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_seq)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/seq.vhd");

   const error_t expect[] = {
      {  15, "type of test must be BOOLEAN" },
      {  19, "no visible declaration for X" },
      {  25, "name TRUE cannot be used in this context" },
      {  32, "no visible declaration for X" },
      {  48, "return statement not allowed outside subprogram" },
      {  62, "return statement not allowed outside subprogram" },
      {  64, "type of loop condition must be BOOLEAN" },
      {  79, "no visible declaration for X" },
      { 106, "others choice must appear last" },
      { 113, "case choice must be locally static" },
      { 126, "case choice must be locally static" },
      { 136, "case choice must be locally static" },
      { 139, "invalid use of BIT" },
      { 146, "type mismatch in range" },
      { 150, "case choice range must have type INTEGER" },
      { 154, "right index of case choice range is not locally static" },
      { 164, "type of exit condition must be BOOLEAN" },
      { 179, "cannot associate this expression with parameter" },
      { 187, "type of next condition must be BOOLEAN" },
      { 190, "cannot use next outside loop" },
      { 192, "no nested loop with label FOO" },
      { 205, "DUP already declared in this region" },
      { 214, "type REAL does not have a range" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_conc)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/conc.vhd");

   const error_t expect[] = {
      { 12, "name '4' cannot be used in this context" },
      { 16, "type of condition must be BOOLEAN" },
      { 18, "reject interval must have type TIME" },
      { 26, "choice must be locally static" },
      { 29, "name TRUE cannot be used in this context" },
      { 32, "name FALSE cannot be used in this context" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_procedure)
{
   tree_t p;

   input_from_file(TESTDIR "/sem/procedure.vhd");

   const error_t expect[] = {
      {   5, "subprogram body is not allowed in package specification" },
      {  28, "cannot return a value from a procedure" },
      {  45, "type of default value universal integer does not match" },
      {  63, "no matching procedure DIFF_TYPES" },
      {  64, "positional parameters must precede named parameters" },
      {  83, "cannot read output port Y" },
      {  84, "invalid target of variable assignment" },
      {  90, "implicit signal STABLE cannot be used in a subprogram body" },
      {  91, "implicit signal QUIET cannot be used in a subprogram body" },
      {  92, "implicit signal TRANSACTION cannot be used in a subprogram" },
      {  93, "implicit signal DELAYED cannot be used in a subprogram" },
      {  98, "object X with access type must have class VARIABLE" },
      {  99, "object X with access type must have class VARIABLE" },
      { 100, "object X with access type must have class VARIABLE" },
      { 137, "sorry, this form of parameter name is not yet supported" },
      { 142, "cannot read output port X" },
      { 148, "target of signal assignment is not a signal" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   fail_if(sem_check(p));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_concat)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/concat.vhd");

   const error_t expect[] = {
      { 24, "type of scalar does not match element type of array" },
      { 26, "cannot concatenate values of different types" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_conv)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/conv.vhd");

   const error_t expect[] = {
      { 28, "conversion only allowed between closely related types" },
      { 29, "type of value B does not match type of target A" },
      { 31, "conversion only allowed between closely related types" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_attr)
{
   tree_t a, e, p;

   input_from_file(TESTDIR "/sem/attr.vhd");

   const error_t expect[] = {
      {  30, "Z has no attribute FOO" },
      {  52, "invalid attribute reference" },
      {  54, "prefix of user defined attribute reference cannot denote" },
      {  65, "expected attribute type INTEGER" },
      {  66, "expected attribute type STRING" },
      {  67, "no visible declaration for Q" },
      {  85, "parameter must be locally static" },
      { 127, "cannot index non-array type universal integer" },
      { 133, "class of object I is variable not signal" },
      { 140, "prefix of attribute LAST_EVENT must denote a signal" },
      { 152, "type BIT_VECTOR does not have a range" },
      { 153, "object B does not have a range" },
      { 154, "object E does not have a range" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   fail_unless(sem_check(p));

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   fail_unless(sem_check(p));

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   fail_unless(sem_check(e));

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_generate)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/generate.vhd");

   const error_t expect[] = {
      { 15, "condition of generate statement must be BOOLEAN" },
      { 26, "no visible declaration for Y" },
      { 45, "condition of generate statement must be static" },
      { 48, "range of generate statement must be static" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   fail_if(sem_check(a));

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_record)
{
   tree_t p;

   input_from_file(TESTDIR "/sem/record.vhd");

   const error_t expect[] = {
      {  9, "duplicate field name X" },
      { 15, "recursive record types are not allowed" },
      { 30, "field X with unconstrained array type is not allowed" },
      { 39, "field Z does not have a value" },
      { 40, "does not match type of field Y" },
      { 42, "field Y does not have a value" },
      { 43, "type R1 does not have field named Q" },
      { 44, "type of value R1 does not match type of" },
      { 47, "field X already has a value" },
      { 48, "field X already has a value" },
      { 64, "type R1_VEC is unconstrained" },
      { 72, "record type R1 has no field F" },
      { 82, "no visible declaration for A.Z" },
      { 86, "record subtype may not have constraints" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_file)
{
   tree_t p;

   input_from_file(TESTDIR "/sem/file.vhd");

   const error_t expect[] = {
      {  6, "files may not be of access type" },
      {  8, "files may not be of file type" },
      { 12, "file declarations must have file type" },
      { 16, "open mode must have type FILE_OPEN_KIND" },
      { 20, "file name must have type STRING" },
      { 36, "no suitable overload for procedure READ" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_access)
{
   tree_t p;

   input_from_file(TESTDIR "/sem/access.vhd");

   const error_t expect[] = {
      {  5, "type FOO is not declared" },
      { 34, "null expression must have access type" },
      { 38, "invalid allocator expression" },
      { 39, "name I does not refer to a type" },
      { 41, "does not match type of target INT_PTR" },
      { 47, "type of value REC does not match type of" },
      { 55, "type of allocator expresion INTEGER does not match" },
      { 56, "name S does not refer to a type" },
      { -1, NULL }
   };
   expect_errors(expect);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACK_BODY);
   sem_check(p);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_real)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/real.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   const error_t expect[] = {
      { 16, "type of value MY_REAL does not match type of target" },
      { 25, "conversion only allowed between closely related types" },
      { -1, NULL }
   };
   expect_errors(expect);

   sem_check(a);
   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_entity)
{
   tree_t a1, a2, ca, e, p;

   input_from_file(TESTDIR "/sem/entity.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   fail_unless(tree_stmts(e) == 1);
   ca = tree_stmt(e, 0);
   fail_unless(tree_kind(ca) == T_CASSERT);
   sem_check(e);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   a1 = parse();
   fail_if(a1 == NULL);
   fail_unless(tree_kind(a1) == T_ARCH);

   a2 = parse();
   fail_if(a2 == NULL);
   fail_unless(tree_kind(a2) == T_ARCH);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   const error_t expect[] = {
      { 26, "unit WORK.PACK is not an entity" },
      { 30, "unit WORK.PACK is not an entity" },
      { -1, NULL }
   };
   expect_errors(expect);

   sem_check(a1);
   sem_check(a2);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_signal)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/signal.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   const error_t expect[] = {
      { 14, "no composite type in context" },
      { 15, "no one dimensional arrays of character type in context" },
      { 16, "not a suitable l-value" },
      { 17, "others association not allowed in aggregate signal target" },
      { 18, "cannot assign to input port P" },
      { 22, "no composite type in context" },
      { 23, "not a suitable l-value" },
      { 24, "others association not allowed in aggregate signal target" },
      { 25, "cannot assign to input port P" },
      { 30, "aggregate element must be locally static name" },
      { -1, NULL }
   };
   expect_errors(expect);

   sem_check(a);
   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_static)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/static.vhd");

   const error_t expect[] = {
      { 36, "case choice must be locally static" },
      { 42, "case choice must be locally static" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_subtype)
{
   tree_t p;

   input_from_file(TESTDIR "/sem/subtype.vhd");

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   const error_t expect[] = {
      { 16, "undefined resolution function NOT_HERE" },
      { -1, NULL }
   };
   expect_errors(expect);

   sem_check(p);
   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_universal)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/universal.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   sem_check(e);

   const error_t expect[] = {
      { 12, "no suitable overload for operator \"*\" [REAL" },
      { 14, "no suitable overload for operator \"*\" [INTEGER" },
      { 16, "no suitable overload for operator \"/\" [universal real" },
      { -1, NULL }
   };
   expect_errors(expect);

   sem_check(a);
   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_issue52)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/issue52.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_issue58)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/issue58.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_spec)
{
   tree_t a, e, p;

   input_from_file(TESTDIR "/sem/spec.vhd");

   const error_t expect[] = {
      { 22, "component mismatch for instance I1: expected C1" },
      { 24, "object E is not a component declaration" },
      { 28, "instance BAD not found" },
      { 30, "specification may only be used with component instances" },
      { 32, "instance I1 is already bound by a specification" },
      { 34, "cannot find unit WORK.NOT_HERE" },
      { 36, "unit WORK.P is not an entity" },
      { 40, "instance I5 not found" },
      { 42, "instance I4 is already bound by a specification" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_issue53)
{
   tree_t e;

   input_from_file(TESTDIR "/sem/issue53.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_supersede)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/supersede.vhd");

   const error_t expect[] = {
      { 18, "WORK.PORTLISTTEST has no formal A" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_implicit)
{
   tree_t a, e;

   input_from_file(TESTDIR "/sem/implicit.vhd");

   const error_t expect[] = {
      { 12, "expected type TIME for attribute DELAYED" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_config)
{
   tree_t a, e, c;

   input_from_file(TESTDIR "/sem/config.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   for (int i = 0; i < 3; i++) {
      e = parse();
      fail_if(e == NULL);
      fail_unless(tree_kind(e) == T_ENTITY);
      sem_check(e);

      a = parse();
      fail_if(a == NULL);
      fail_unless(tree_kind(a) == T_ARCH);
      sem_check(a);
   }

   c = parse();
   fail_if(c == NULL);
   fail_unless(tree_kind(c) == T_CONFIG);
   sem_check(c);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_protected)
{
   tree_t e, a, p;

   set_standard(STD_00);

   input_from_file(TESTDIR "/sem/protected.vhd");

   const error_t expect[] = {
      {  13, "type NOT_HERE is not declared" },
      {  16, "no protected type declaration for BAD2 found" },
      {  19, "object INTEGER is not a protected type declaration" },
      {  22, "object NOW is not a protected type declaration" },
      {  44, "protected type SHAREDCOUNTER already has body" },
      {  47, "subtypes may not have protected base types" },
      {  49, "shared variable X must have protected type" },
      {  53, "variable Y with protected type may not have an initial value" },
      { 105, "no visible declaration for X.COUNTER" },
      { 106, "no suitable overload for procedure X.DECREMENT" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   p = parse();
   fail_if(p == NULL);
   fail_unless(tree_kind(p) == T_PACKAGE);
   sem_check(p);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_alias)
{
   tree_t e, a;

   input_from_file(TESTDIR "/sem/alias.vhd");

   const error_t expect[] = {
      { 10, "non-object alias may not have subtype indication" },
      { 12, "name X does not refer to a type" },
      { 22, "no visible subprogram FOO matches signature [INTEGER "
        "return INTEGER]" },
      { 23, "no visible subprogram FOO matches signature [BIT]" },
      { 24, "invalid name in subprogram alias" },
      { 25, "type BLAH is not declared" },
      { 32, "no visible subprogram BAR matches signature [INTEGER]" },
      { 40, "ambiguous use of enumeration literal '1'" },
      { 41, "no visible declaration for FOO_INT" },
      { 42, "no suitable overload for procedure BAR_BIT [CHARACTER]" },
      { -1, NULL }
   };
   expect_errors(expect);

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);

   fail_unless(sem_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_issue102)
{
   input_from_file(TESTDIR "/sem/issue102.vhd");

   for (int i = 0; i < 3; i++) {
      tree_t t = parse();
      fail_if(t == NULL);
      sem_check(t);
   }

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);
}
END_TEST

START_TEST(test_issue105)
{
   set_standard(STD_08);

   input_from_file(TESTDIR "/sem/issue105.vhd");

   for (int i = 0; i < 4; i++) {
      tree_t t = parse();
      fail_if(t == NULL);
      sem_check(t);
   }

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);
}
END_TEST

int main(void)
{
   register_trace_signal_handlers();

   setenv("NVC_LIBPATH", "../lib/std", 1);

   Suite *s = suite_create("sem");

   TCase *tc_core = tcase_create("Core");
   tcase_add_unchecked_fixture(tc_core, setup, teardown);
   tcase_add_test(tc_core, test_integer);
   tcase_add_test(tc_core, test_ports);
   tcase_add_test(tc_core, test_scope);
   tcase_add_test(tc_core, test_ambiguous);
   tcase_add_test(tc_core, test_const);
   tcase_add_test(tc_core, test_std);
   tcase_add_test(tc_core, test_wait);
   tcase_add_test(tc_core, test_func);
   tcase_add_test(tc_core, test_array);
   tcase_add_test(tc_core, test_assert);
   tcase_add_test(tc_core, test_generics);
   tcase_add_test(tc_core, test_seq);
   tcase_add_test(tc_core, test_conc);
   tcase_add_test(tc_core, test_procedure);
   tcase_add_test(tc_core, test_concat);
   tcase_add_test(tc_core, test_conv);
   tcase_add_test(tc_core, test_attr);
   tcase_add_test(tc_core, test_generate);
   tcase_add_test(tc_core, test_record);
   tcase_add_test(tc_core, test_file);
   tcase_add_test(tc_core, test_access);
   tcase_add_test(tc_core, test_real);
   tcase_add_test(tc_core, test_entity);
   tcase_add_test(tc_core, test_signal);
   tcase_add_test(tc_core, test_static);
   tcase_add_test(tc_core, test_subtype);
   tcase_add_test(tc_core, test_universal);
   tcase_add_test(tc_core, test_issue52);
   tcase_add_test(tc_core, test_issue58);
   tcase_add_test(tc_core, test_spec);
   tcase_add_test(tc_core, test_issue53);
   tcase_add_test(tc_core, test_supersede);
   tcase_add_test(tc_core, test_implicit);
   tcase_add_test(tc_core, test_config);
   tcase_add_test(tc_core, test_protected);
   tcase_add_test(tc_core, test_alias);
   tcase_add_test(tc_core, test_issue102);
   tcase_add_test(tc_core, test_issue105);
   suite_add_tcase(s, tc_core);

   SRunner *sr = srunner_create(s);
   srunner_run_all(sr, CK_NORMAL);

   int nfail = srunner_ntests_failed(sr);

   srunner_free(sr);

   return nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
