#include "test_util.h"
#include "phase.h"
#include "vcode.h"

#include <inttypes.h>

typedef struct {
   vcode_op_t    op;
   const char   *func;
   int64_t       value;
   vcode_cmp_t   cmp;
   vcode_block_t target;
   vcode_block_t target_else;
   const char   *name;
   bool          delay;
   int64_t       low;
   int64_t       high;
   int           length;
   int           args;
   int           dim;
   int           hops;
   int           field;
   int           subkind;
} check_bb_t;

#define CAT(x, y) x##y
#define CHECK_BB(n) check_bb(n, CAT(bb, n), ARRAY_LEN(CAT(bb, n)))
#define EXPECT_BB(n) const check_bb_t CAT(bb, n)[]

static void check_bb(int bb, const check_bb_t *expect, int len)
{
   fail_unless(bb < vcode_count_blocks());
   vcode_select_block(bb);

   const int nops = vcode_count_ops();

   int eptr = 0, actual = nops;
   for (int i = 0; i < nops && eptr < len; i++) {
      const vcode_op_t vop = vcode_get_op(i);
      if (vop == VCODE_OP_COMMENT) {
         actual--;
         continue;
      }

      const check_bb_t *e = &(expect[eptr++]);

      if (vop != e->op) {
         vcode_dump();
         fail("expected op %d in block %d to be %s but was %s",
              i, bb, vcode_op_string(e->op), vcode_op_string(vop));
      }

      switch (e->op) {
      case VCODE_OP_PCALL:
         if (e->target != vcode_get_target(i, 0)) {
            vcode_dump();
            fail("expected op %d in block %d to have resume target %d but "
                 "has %d", i, bb, e->target, vcode_get_target(i, 0));
         }
         // Fall-through
      case VCODE_OP_FCALL:
      case VCODE_OP_NESTED_FCALL:
         if ((e->func != NULL) && !icmp(vcode_get_func(i), e->func)) {
            vcode_dump();
            fail("expected op %d in block %d to call %s but calls %s",
                 i, bb, e->func, istr(vcode_get_func(i)));
         }
         else if (e->args != vcode_count_args(i)) {
            vcode_dump();
            fail("expected op %d in block %d to have %d arguments but has %d",
                 i, bb, e->args, vcode_count_args(i));
         }
         break;

      case VCODE_OP_CONST:
         if (e->value != vcode_get_value(i)) {
            vcode_dump();
            fail("expected op %d in block %d to have constant %d but has %d",
                 i, bb, e->value, vcode_get_value(i));
         }
         break;

      case VCODE_OP_CMP:
         if (e->cmp != vcode_get_cmp(i)) {
            vcode_dump();
            fail("expected op %d in block %d to have comparison %d but has %d",
                 i, bb, e->value, vcode_get_cmp(i));
         }
         break;

      case VCODE_OP_ASSERT:
      case VCODE_OP_REPORT:
      case VCODE_OP_RETURN:
      case VCODE_OP_IMAGE:
      case VCODE_OP_UNWRAP:
         break;

      case VCODE_OP_UARRAY_LEFT:
      case VCODE_OP_UARRAY_RIGHT:
      case VCODE_OP_UARRAY_DIR:
         if (e->dim != vcode_get_dim(i)) {
            vcode_dump();
            fail("expected op %d in block %d to have dimension %d but has %d",
                 i, bb, e->dim, vcode_get_dim(i));
         }
         break;

      case VCODE_OP_WAIT:
         if (e->target != vcode_get_target(i, 0)) {
            vcode_dump();
            fail("expected op %d in block %d to have wait target %d but has %d",
                 i, bb, e->target, vcode_get_target(i, 0));
         }
         else if (e->delay && vcode_get_arg(i, 0) == VCODE_INVALID_REG) {
            vcode_dump();
            fail("expected op %d in block %d to have wait delay", i, bb);
         }
         break;

      case VCODE_OP_COND:
         if (e->target_else != vcode_get_target(i, 1)) {
            vcode_dump();
            fail("expected op %d in block %d to have else target %d but has %d",
                 i, bb, e->target, vcode_get_target(i, 1));
         }
         // Fall-through

      case VCODE_OP_JUMP:
         if (e->target != vcode_get_target(i, 0)) {
            vcode_dump();
            fail("expected op %d in block %d to have jump target %d but has %d",
                 i, bb, e->target, vcode_get_target(i, 0));
         }
         break;

      case VCODE_OP_STORE:
      case VCODE_OP_LOAD:
      case VCODE_OP_INDEX:
         {
            ident_t name = vcode_var_name(vcode_get_address(i));
            if (name != ident_new(e->name)) {
               vcode_dump();
               fail("expected op %d in block %d to have address name %s but "
                    "has %s", i, bb, e->name, istr(name));
            }
         }
         break;

      case VCODE_OP_NETS:
      case VCODE_OP_RESOLVED_ADDRESS:
         {
            ident_t name = vcode_signal_name(vcode_get_signal(i));
            if (name != ident_new(e->name)) {
               vcode_dump();
               fail("expected op %d in block %d to have signal name %s but "
                    "has %s", i, bb, e->name, istr(name));
            }
         }
         break;

      case VCODE_OP_ADD:
      case VCODE_OP_SUB:
      case VCODE_OP_MUL:
      case VCODE_OP_DIV:
      case VCODE_OP_EXP:
      case VCODE_OP_MOD:
      case VCODE_OP_REM:
      case VCODE_OP_NEG:
      case VCODE_OP_ABS:
      case VCODE_OP_CAST:
      case VCODE_OP_OR:
      case VCODE_OP_AND:
      case VCODE_OP_NOT:
      case VCODE_OP_LOAD_INDIRECT:
      case VCODE_OP_STORE_INDIRECT:
      case VCODE_OP_SCHED_WAVEFORM:
      case VCODE_OP_ALLOCA:
      case VCODE_OP_SELECT:
      case VCODE_OP_SET_INITIAL:
      case VCODE_OP_ALLOC_DRIVER:
      case VCODE_OP_EVENT:
      case VCODE_OP_ACTIVE:
      case VCODE_OP_CONST_RECORD:
      case VCODE_OP_COPY:
      case VCODE_OP_MEMCMP:
      case VCODE_OP_MEMSET:
      case VCODE_OP_WRAP:
      case VCODE_OP_VEC_LOAD:
      case VCODE_OP_DYNAMIC_BOUNDS:
      case VCODE_OP_NEW:
      case VCODE_OP_ALL:
      case VCODE_OP_DEALLOCATE:
         break;

      case VCODE_OP_CONST_ARRAY:
         if (vcode_count_args(i) != e->length) {
            vcode_dump();
            fail("expected op %d in block %d to have %d array elements but "
                 "has %d", i, bb, e->length, vcode_count_args(i));
         }
         break;

      case VCODE_OP_BOUNDS:
         {
            vcode_type_t bounds = vcode_get_type(i);
            if (vtype_kind(bounds) == VCODE_TYPE_INT) {
               if (e->low != vtype_low(bounds)) {
                  vcode_dump();
                  fail("expect op %d in block %d to have low bound %"PRIi64
                       " but has %"PRIi64, i, bb, e->low, vtype_low(bounds));
               }
               else if (e->high != vtype_high(bounds)) {
                  vcode_dump();
                  fail("expect op %d in block %d to have high bound %"PRIi64
                       " but has %"PRIi64, i, bb, e->high, vtype_high(bounds));
               }
            }
         }
         break;

      case VCODE_OP_PARAM_UPREF:
         if (vcode_get_hops(i) != e->hops) {
            vcode_dump();
            fail("expect op %d in block %d to have hop count %d"
                 " but has %d", i, bb, e->hops, vcode_get_hops(i));
         }
         break;

      case VCODE_OP_RECORD_REF:
         if (vcode_get_field(i) != e->field) {
            vcode_dump();
            fail("expect op %d in block %d to have field %d"
                 " but has %d", i, bb, e->field, vcode_get_field(i));
         }
         break;

      case VCODE_OP_SCHED_EVENT:
         if (vcode_get_subkind(i) != e->subkind) {
            vcode_dump();
            fail("expected op %d in block %d to have subkind %x but "
                 "has %x", i, bb, e->subkind, vcode_get_subkind(i));
         }
         break;

      case VCODE_OP_RESUME:
         if ((e->func != NULL) && !icmp(vcode_get_func(i), e->func)) {
            vcode_dump();
            fail("expected op %d in block %d to call %s but calls %s",
                 i, bb, e->func, istr(vcode_get_func(i)));
         }
         break;

      default:
         fail("cannot check op %s", vcode_op_string(e->op));
      }
   }

   if (actual != eptr) {
      vcode_dump();
      fail("expected %d ops in block %d but have %d", len, bb, actual);
   }
}

START_TEST(test_wait1)
{
   input_from_file(TESTDIR "/lower/wait1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   const check_bb_t bb0[] = {
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   const check_bb_t bb1[] = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_FCALL, .func = "_std_standard_now" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP,   .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_WAIT,  .target = 2 }
   };

   CHECK_BB(1);

   const check_bb_t bb2[] = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_FCALL, .func = "_std_standard_now" },
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_CMP,   .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_WAIT,  .target = 3 }
   };

   CHECK_BB(2);

   const check_bb_t bb3[] = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_FCALL, .func = "_std_standard_now" },
      { VCODE_OP_CONST, .value = 1000001 },
      { VCODE_OP_CMP,   .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT,  .target = 4 }
   };

   CHECK_BB(3);

   const check_bb_t bb4[] = {
      { VCODE_OP_JUMP,  .target = 1 }
   };

   CHECK_BB(4);
}
END_TEST

START_TEST(test_assign1)
{
   input_from_file(TESTDIR "/lower/assign1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   fail_unless(vcode_count_vars() == 2);

   const check_bb_t bb0[] = {
      { VCODE_OP_CONST, .value = 64 },
      { VCODE_OP_STORE, .name = "X" },
      { VCODE_OP_CONST, .value = -4 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   const check_bb_t bb1[] = {
      { VCODE_OP_CONST, .value = 4000000 },
      { VCODE_OP_WAIT,  .target = 2, .delay = true }
   };

   CHECK_BB(1);

   const check_bb_t bb2[] = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_LOAD,  .name = "X" },
      { VCODE_OP_CONST, .value = 64 },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_LOAD,  .name = "Y" },
      { VCODE_OP_CONST, .value = -4 },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_MUL },
      { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
      { VCODE_OP_CONST,  .value = -8 },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 5 },
      { VCODE_OP_STORE, .name = "X" },
      { VCODE_OP_CONST, .value = 7 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_WAIT,  .target = 3, .delay = true }
   };

   CHECK_BB(2);

   const check_bb_t bb3[] = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_LOAD, .name = "X" },
      { VCODE_OP_LOAD, .name = "Y" },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 12 },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 4 }
   };

   CHECK_BB(3);

   const check_bb_t bb4[] = {
      { VCODE_OP_JUMP, .target = 1 }
   };

   CHECK_BB(4);
}
END_TEST

START_TEST(test_assign2)
{
   input_from_file(TESTDIR "/lower/assign2.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_INDEX, .name = "X" },
      { VCODE_OP_CONST, .value = 8 },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST_ARRAY, .length = 8 },
      { VCODE_OP_COPY },
      { VCODE_OP_INDEX, .name = "Y" },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CONST_ARRAY, .length = 3 },
      { VCODE_OP_COPY },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   for (int i = 0; i < 8; i++)
      fail_unless(vcode_get_arg(5, i) == vcode_get_result((i == 6) ? 3 : 4));

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_INDEX, .name = "X" },
      { VCODE_OP_CONST, .value = 7 },
      { VCODE_OP_ADD },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_ADD },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CONST, .value = 5 },
      { VCODE_OP_ADD },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_INDEX, .name = "Y" },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_ADD },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_signal1)
{
   input_from_file(TESTDIR "/lower/signal1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t vc = tree_code(e);
   vcode_select_unit(vc);

   {
      EXPECT_BB(0) = {
         { VCODE_OP_CONST, .value = 5 },
         { VCODE_OP_SET_INITIAL },
         { VCODE_OP_RESOLVED_ADDRESS, .name = ":signal1:x" },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }

   fail_unless(vcode_count_signals() == 1);
   fail_unless(icmp(vcode_signal_name(0), ":signal1:x"));
   fail_unless(vcode_signal_count_nets(0) == 1);
   fail_unless(vcode_signal_nets(0)[0] == 0);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   {
      EXPECT_BB(0) = {
         { VCODE_OP_NETS, .name = ":signal1:x" },
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_ALLOC_DRIVER },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);

      EXPECT_BB(1) = {
         { VCODE_OP_CONST, .value = 2 },
         { VCODE_OP_LOAD, .name = "resolved_:signal1:x" },
         { VCODE_OP_LOAD_INDIRECT },
         { VCODE_OP_CONST, .value = 5 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
         { VCODE_OP_ASSERT },
         { VCODE_OP_CONST, .value = 0 },
         { VCODE_OP_NETS, .name = ":signal1:x" },
         { VCODE_OP_CONST, .value = 6 },
         { VCODE_OP_CONST, .value = 0 },
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_SCHED_WAVEFORM },
         { VCODE_OP_WAIT, .target = 2 }
      };

      CHECK_BB(1);
   }
}
END_TEST

START_TEST(test_cond1)
{
   input_from_file(TESTDIR "/lower/cond1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_CONST, .value = -2147483648 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_LOAD, .name = "resolved_:cond1:x" },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_LOAD, .name = "Y" },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_COND, .target = 2, .target_else = 3 }
   };

   CHECK_BB(1);

   EXPECT_BB(2) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_JUMP, .target = 3 }
   };

   CHECK_BB(2);

   EXPECT_BB(3) = {
      { VCODE_OP_LOAD, .name = "resolved_:cond1:x" },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_LOAD, .name = "Y" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_COND, .target = 4, .target_else = 5 }
   };

   CHECK_BB(3);

   EXPECT_BB(4) = {
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_JUMP, .target = 6 }
   };

   CHECK_BB(4);

   EXPECT_BB(5) = {
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_JUMP, .target = 6 }
   };

   CHECK_BB(5);

   EXPECT_BB(6) = {
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST, .value = 100 },
      { VCODE_OP_CONST, .value = 111 },
      { VCODE_OP_CONST_ARRAY, .length = 3 },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_REPORT },
      { VCODE_OP_WAIT, .target = 7 }
   };

   CHECK_BB(6);
}
END_TEST

START_TEST(test_arith1)
{
   input_from_file(TESTDIR "/lower/arith1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_CONST, .value = -2147483648 },
      { VCODE_OP_STORE, .name = "X" },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_STORE, .name = "X" },
      { VCODE_OP_CONST, .value = 12 },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);

   EXPECT_BB(2) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_LOAD, .name = "X" },
      { VCODE_OP_LOAD, .name = "Y" },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 15 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_SUB },
      { VCODE_OP_CONST, .value = -9 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_MUL },
      { VCODE_OP_CONST, .value = 36 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 12 },
      { VCODE_OP_DIV },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_NEQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_LT },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_GT },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_LEQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_GEQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_NEG },
      { VCODE_OP_CONST, .value = -3 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_EXP },
      { VCODE_OP_CONST, .value = 531441 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = -34 },
      { VCODE_OP_STORE, .name = "X" },
      { VCODE_OP_ABS },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 5 },
      { VCODE_OP_MOD },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_REM },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = -5 },
      { VCODE_OP_REM },
      { VCODE_OP_CONST, .value = -2 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_MOD },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 3 }
   };

   CHECK_BB(2);
}
END_TEST

START_TEST(test_pack1)
{
   input_from_file(TESTDIR "/lower/pack1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t t, body = NULL;
   while ((t = parse())) {
      sem_check(t);
      fail_if(sem_errors() > 0);

      simplify(t);

      if (tree_kind(t) == T_PACK_BODY)
         body = t;
   }

   fail_if(body == NULL);
   lower_unit(body);

   tree_t add1 = tree_decl(body, 0);
   fail_unless(tree_kind(add1) == T_FUNC_BODY);

   vcode_unit_t v0 = tree_code(add1);
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);
}
END_TEST

START_TEST(test_func1)
{
   input_from_file(TESTDIR "/lower/func1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_CONST, .value = INT32_MIN },
      { VCODE_OP_STORE, .name = "R" },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
#if LLVM_MANGLES_NAMES
      { VCODE_OP_FCALL, .func = ":func1:add1__II", .args = 1 },
#else
      { VCODE_OP_FCALL, .func = ":func1:add1$II", .args = 1 },
#endif
      { VCODE_OP_STORE, .name = "R" },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_issue94)
{
   input_from_file(TESTDIR "/lower/issue94.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);
}
END_TEST

START_TEST(test_arrayop1)
{
   input_from_file(TESTDIR "/lower/arrayop1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_INDEX, .name = "X" },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST_ARRAY, .length = 3 },
      { VCODE_OP_COPY },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_INDEX, .name = "X" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST_ARRAY, .length = 3 },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_ALLOCA },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_ALLOCA },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_JUMP, .target = 2 }
   };

   CHECK_BB(1);

   EXPECT_BB(2) = {
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_GEQ },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_COND, .target = 4, .target_else = 3 }
   };

   CHECK_BB(2);

   EXPECT_BB(3) = {
      { VCODE_OP_ADD },
      { VCODE_OP_ADD },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_LT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_NOT },
      { VCODE_OP_OR },
      { VCODE_OP_COND, .target = 4, .target_else = 2 }
   };

   CHECK_BB(3);

   EXPECT_BB(4) = {
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 5 }
   };

   CHECK_BB(4);
}
END_TEST

START_TEST(test_array1)
{
   input_from_file(TESTDIR "/lower/array1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_FCALL, .name = ":array1:func" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST_ARRAY, .length = 2 },
      { VCODE_OP_UNWRAP },
      { VCODE_OP_UARRAY_LEFT, .dim = 0 },
      { VCODE_OP_UARRAY_RIGHT, .dim = 0 },
      { VCODE_OP_SUB },
      { VCODE_OP_SUB },
      { VCODE_OP_UARRAY_DIR },
      { VCODE_OP_SELECT },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_LT },
      { VCODE_OP_SELECT },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_MEMCMP },
      { VCODE_OP_AND },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_nest1)
{
   input_from_file(TESTDIR "/lower/nest1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   tree_t p = tree_stmt(e, 0);

   {
      vcode_unit_t v0 = tree_code(p);
      vcode_select_unit(v0);

      EXPECT_BB(1) = {
         { VCODE_OP_CONST, .value = 2 },
         { VCODE_OP_CONST, .value = 5 },
#if LLVM_MANGLES_NAMES
         { VCODE_OP_NESTED_FCALL, .func = ":nest1:line_7_ADD_TO_X__II",
           .args = 1 },
#else
         { VCODE_OP_NESTED_FCALL, .func = ":nest1:line_7_ADD_TO_X$II",
           .args = 1 },
#endif
         { VCODE_OP_CONST, .value = 7 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
         { VCODE_OP_ASSERT },
         { VCODE_OP_WAIT, .target = 2 }
      };

      CHECK_BB(1);
   }

   tree_t f1 = tree_decl(p, 2);
   fail_unless(tree_kind(f1) == T_FUNC_BODY);

   {
      vcode_unit_t v0 = tree_code(f1);
      vcode_select_unit(v0);

#if LLVM_MANGLES_NAMES
      fail_unless(icmp(vcode_unit_name(), ":nest1:line_7_ADD_TO_X__II"));
#else
      fail_unless(icmp(vcode_unit_name(), ":nest1:line_7_ADD_TO_X$II"));
#endif

      EXPECT_BB(0) = {
#if LLVM_MANGLES_NAMES
         { VCODE_OP_NESTED_FCALL, .func = ":nest1:line_7_ADD_TO_X_DO_IT__I" },
#else
         { VCODE_OP_NESTED_FCALL, .func = ":nest1:line_7_ADD_TO_X_DO_IT$I" },
#endif
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }

   tree_t f2 = tree_decl(f1, 0);
   fail_unless(tree_kind(f2) == T_FUNC_BODY);

   {
      vcode_unit_t v0 = tree_code(f2);
      vcode_select_unit(v0);

#if LLVM_MANGLES_NAMES
      fail_unless(icmp(vcode_unit_name(), ":nest1:line_7_ADD_TO_X_DO_IT__I"));
#else
      fail_unless(icmp(vcode_unit_name(), ":nest1:line_7_ADD_TO_X_DO_IT$I"));
#endif

      EXPECT_BB(0) = {
         { VCODE_OP_LOAD, .name = "X" },
         { VCODE_OP_PARAM_UPREF, .hops = 1 },
         { VCODE_OP_ADD },
         { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }

}
END_TEST

START_TEST(test_signal2)
{
   input_from_file(TESTDIR "/lower/signal2.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_NETS, .name = ":signal2:x" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_EVENT },
      { VCODE_OP_ASSERT },
      { VCODE_OP_ACTIVE },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_attr1)
{
   input_from_file(TESTDIR "/lower/attr1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_LOAD, .name = "X" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_SUB },
      { VCODE_OP_CONST, .value = -1 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_LOAD, .name = "Z" },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_LOAD, .name = "Y" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = -1 },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_assign3)
{
   input_from_file(TESTDIR "/lower/assign3.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(1) = {
      { VCODE_OP_INDEX, .name = "X" },
      { VCODE_OP_CONST, .value = 8 },
      { VCODE_OP_INDEX, .name = "Y" },
      { VCODE_OP_COPY },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_MEMCMP },
      { VCODE_OP_NOT },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 2 },
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_record1)
{
   input_from_file(TESTDIR "/lower/record1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_INDEX, .name = "A" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CONST_RECORD },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_INDEX, .name = "B" },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_INDEX, .name = "A" },
      { VCODE_OP_RECORD_REF, .field = 0 },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 5 },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_INDEX, .name = "B" },
      { VCODE_OP_COPY },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_RECORD_REF, .field = 0 },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_RECORD_REF, .field = 1 },
      { VCODE_OP_RECORD_REF, .field = 1 },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_AND },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_signal4)
{
   input_from_file(TESTDIR "/lower/signal4.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(1) = {
      { VCODE_OP_LOAD, .name = "resolved_:signal4:s" },
      { VCODE_OP_LOAD_INDIRECT},
      { VCODE_OP_INDEX, .name = "V" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_NETS, .name = ":signal4:s" },
      { VCODE_OP_CONST, .value = 4 },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_SCHED_WAVEFORM },
      { VCODE_OP_COPY },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_staticwait)
{
   input_from_file(TESTDIR "/lower/staticwait.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_NETS, .name = ":staticwait:x" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ALLOC_DRIVER },
      { VCODE_OP_SCHED_EVENT, .subkind = 2 },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_NETS, .name = ":staticwait:x" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_SCHED_WAVEFORM },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_proc1)
{
   input_from_file(TESTDIR "/lower/proc1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   opt(e);
   lower_unit(e);

   {
      vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
      vcode_select_unit(v0);

      EXPECT_BB(1) = {
         { VCODE_OP_LOAD, .name = "A" },
         { VCODE_OP_INDEX, .name = "B" },
#if LLVM_MANGLES_NAMES
         { VCODE_OP_FCALL, .func = ":proc1:add1__II", .args = 2 },
#else
         { VCODE_OP_FCALL, .func = ":proc1:add1$II", .args = 2 },
#endif
         { VCODE_OP_CONST, .value = 2 },
         { VCODE_OP_LOAD, .name = "B" },
         { VCODE_OP_CONST, .value = 3 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
         { VCODE_OP_ASSERT },
         { VCODE_OP_CONST, .value = 5 },
#if LLVM_MANGLES_NAMES
         { VCODE_OP_FCALL, .func = ":proc1:add1__II", .args = 2 },
#else
         { VCODE_OP_FCALL, .func = ":proc1:add1$II", .args = 2 },
#endif
         { VCODE_OP_LOAD, .name = "B" },
         { VCODE_OP_CONST, .value = 6 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
         { VCODE_OP_ASSERT },
         { VCODE_OP_WAIT, .target = 2 }
      };

      CHECK_BB(1);
   }

   {
      vcode_unit_t v0 = tree_code(tree_decl(e, 1));
      vcode_select_unit(v0);

      EXPECT_BB(0) = {
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_ADD },
         { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
         { VCODE_OP_STORE_INDIRECT },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }
}
END_TEST

START_TEST(test_while1)
{
   input_from_file(TESTDIR "/lower/while1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(2) = {
      { VCODE_OP_LOAD, .name = "N" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_GT },
      { VCODE_OP_COND, .target = 3, .target_else = 4 }
   };

   CHECK_BB(2);

   EXPECT_BB(3) = {
      { VCODE_OP_LOAD, .name = "N" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_SUB },
      { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
      { VCODE_OP_STORE, .name = "N" },
      { VCODE_OP_JUMP, .target = 2 }
   };

   CHECK_BB(3);
}
END_TEST

START_TEST(test_loop1)
{
   input_from_file(TESTDIR "/lower/loop1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(3) = {
      { VCODE_OP_LOAD, .name = "A" },
      { VCODE_OP_CONST, .value = 10 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_COND, .target = 6, .target_else = 5 }
   };

   CHECK_BB(3);

   EXPECT_BB(8) = {
      { VCODE_OP_LOAD, .name = "A" },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
      { VCODE_OP_STORE, .name = "A" },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_MOD },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_COND, .target = 11, .target_else = 10 }
   };

   CHECK_BB(8);
}
END_TEST

START_TEST(test_proc3)
{
   input_from_file(TESTDIR "/lower/proc3.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   {
      vcode_unit_t v0 = tree_code(tree_decl(e, 1));
      vcode_select_unit(v0);

      EXPECT_BB(0) = {
         { VCODE_OP_CONST, .value = 10000000 },
         { VCODE_OP_WAIT, .delay = true, .target = 1 }
      };

      CHECK_BB(0);

      EXPECT_BB(1) = {
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_STORE_INDIRECT },
         { VCODE_OP_CONST, .value = 5000000 },
         { VCODE_OP_WAIT, .delay = true, .target = 2 }
      };

      CHECK_BB(1);
   }

   {
      vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
      vcode_select_unit(v0);

      EXPECT_BB(1) = {
         { VCODE_OP_INDEX, .name = "X" },
#if LLVM_MANGLES_NAMES
         { VCODE_OP_PCALL, .func = ":proc3:p1__I", .target = 2, .args = 1 },
#else
         { VCODE_OP_PCALL, .func = ":proc3:p1$I", .target = 2, .args = 1 }
#endif
      };

      CHECK_BB(1);

      EXPECT_BB(2) = {
#if LLVM_MANGLES_NAMES
         { VCODE_OP_RESUME, .func = ":proc3:p1__I" },
#else
         { VCODE_OP_RESUME, .func = ":proc3:p1$I" },
#endif
         { VCODE_OP_WAIT, .target = 3 }
      };

      CHECK_BB(2);
   }
}
END_TEST

START_TEST(test_loop2)
{
   input_from_file(TESTDIR "/lower/loop2.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_decl(e, 1));
   vcode_select_unit(v0);

   EXPECT_BB(2) = {
      { VCODE_OP_CONST, .value = 1000 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_GEQ },
      { VCODE_OP_COND, .target = 4, .target_else = 5 }
   };

   CHECK_BB(2);

   EXPECT_BB(3) = {
      { VCODE_OP_RETURN },
   };

   CHECK_BB(3);
}
END_TEST

START_TEST(test_slice1)
{
   input_from_file(TESTDIR "/lower/slice1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(1) = {
      { VCODE_OP_INDEX, .name = "V" },
      { VCODE_OP_CONST, .value = 4 },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_CONST, .value = 4 },
      { VCODE_OP_CONST_ARRAY, .length = 4 },
      { VCODE_OP_COPY },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_CONST, .value = 6 },
      { VCODE_OP_CONST, .value = 7 },
      { VCODE_OP_CONST_ARRAY, .length = 2 },
      { VCODE_OP_COPY },
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_ADD },
      { VCODE_OP_CONST_ARRAY, .length = 2 },
      { VCODE_OP_MEMCMP },
      { VCODE_OP_ASSERT },
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_WAIT, .target = 2 },
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_funcif)
{
   input_from_file(TESTDIR "/lower/funcif.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_decl(e, 1));
   vcode_select_unit(v0);

   fail_unless(vcode_count_blocks() == 3);
}
END_TEST

START_TEST(test_memset)
{
   input_from_file(TESTDIR "/lower/memset.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   {
      vcode_unit_t v0 = tree_code(tree_decl(e, 1));
      vcode_select_unit(v0);

      EXPECT_BB(0) = {
         { VCODE_OP_CONST, .value = 0 },
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_SUB },
         { VCODE_OP_ADD },
         { VCODE_OP_CAST },
         { VCODE_OP_CONST, .value = 0 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_LT },
         { VCODE_OP_SELECT },
         { VCODE_OP_ALLOCA },
         { VCODE_OP_MEMSET },
         { VCODE_OP_WRAP },
         { VCODE_OP_STORE, .name = "V" },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }

   {
      vcode_unit_t v0 = tree_code(tree_decl(e, 2));
      vcode_select_unit(v0);

      EXPECT_BB(0) = {
         { VCODE_OP_CONST, .value = 0 },
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_SUB },
         { VCODE_OP_ADD },
         { VCODE_OP_CAST },
         { VCODE_OP_CONST, .value = 0 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_LT },
         { VCODE_OP_SELECT },
         { VCODE_OP_ALLOCA },
         { VCODE_OP_CONST, .value = 0xab },
         { VCODE_OP_CONST, .value = 4 },
         { VCODE_OP_MUL },
         { VCODE_OP_MEMSET },
         { VCODE_OP_WRAP },
         { VCODE_OP_STORE, .name = "V" },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }
}
END_TEST

START_TEST(test_func5)
{
   input_from_file(TESTDIR "/lower/func5.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   {
      vcode_unit_t v0 = tree_code(tree_decl(e, 1));
      vcode_select_unit(v0);

      EXPECT_BB(0) = {
         { VCODE_OP_VEC_LOAD },
         { VCODE_OP_LOAD_INDIRECT },
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_ADD },
         { VCODE_OP_BOUNDS, .low = INT32_MIN, .high = INT32_MAX },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }

   {
      vcode_unit_t v0 = tree_code(tree_decl(e, 2));
      vcode_select_unit(v0);

      EXPECT_BB(0) = {
         { VCODE_OP_CONST, .value = 1 },
         { VCODE_OP_EVENT },
         { VCODE_OP_RETURN }
      };

      CHECK_BB(0);
   }

   {
      vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
      vcode_select_unit(v0);

      EXPECT_BB(1) = {
         { VCODE_OP_CONST, .value = 2 },
         { VCODE_OP_NETS, .name = ":func5:x" },
#if LLVM_MANGLES_NAMES
         { VCODE_OP_FCALL, .func = ":func5:add_one_s__IsI", .args = 1 },
#else
         { VCODE_OP_FCALL, .func = ":func5:add_one_s$IsI", .args = 1 },
#endif
         { VCODE_OP_CONST, .value = 6 },
         { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
         { VCODE_OP_ASSERT },
#if LLVM_MANGLES_NAMES
         { VCODE_OP_FCALL, .func = ":func5:event__BsI", .args = 1 },
#else
         { VCODE_OP_FCALL, .func = ":func5:event$BsI", .args = 1 },
#endif
         { VCODE_OP_ASSERT },
         { VCODE_OP_WAIT, .target = 2 }
      };

      CHECK_BB(1);
   }
}
END_TEST

START_TEST(test_bounds1)
{
   input_from_file(TESTDIR "/lower/bounds1.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_stmt(e, 0));
   vcode_select_unit(v0);

   EXPECT_BB(1) = {
      { VCODE_OP_CONST, .value = 2 },
      { VCODE_OP_INDEX, .name = "V" },
      { VCODE_OP_LOAD,  .name = "K" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST, .value = 9 },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CAST },
      { VCODE_OP_ADD },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CONST, .value = 1 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_ADD },
      { VCODE_OP_DYNAMIC_BOUNDS },
      { VCODE_OP_CAST },
      { VCODE_OP_ADD },
      { VCODE_OP_LOAD_INDIRECT },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_EQ },
      { VCODE_OP_ASSERT },
      { VCODE_OP_WAIT, .target = 2 }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_record6)
{
   input_from_file(TESTDIR "/lower/record6.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   opt(e);
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_decl(e, 1));
   vcode_select_unit(v0);

   fail_unless(vcode_var_use_heap(vcode_var_handle(0)));

   EXPECT_BB(0) = {
      { VCODE_OP_INDEX, .name = "R" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CONST_ARRAY, .length = 3 },
      { VCODE_OP_CONST, .value = INT32_MIN },
      { VCODE_OP_CONST_RECORD },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_RECORD_REF, .field = 0 },
      { VCODE_OP_CONST, .value = 3 },
      { VCODE_OP_COPY },
      { VCODE_OP_RECORD_REF, .field = 1 },
      { VCODE_OP_STORE_INDIRECT },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);
}
END_TEST

START_TEST(test_proc7)
{
   input_from_file(TESTDIR "/lower/proc7.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   opt(e);
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_decl(e, 1));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_UARRAY_LEFT },
      { VCODE_OP_CAST },
      { VCODE_OP_UARRAY_RIGHT },
      { VCODE_OP_CAST },
      { VCODE_OP_UARRAY_DIR },
      { VCODE_OP_SUB },
      { VCODE_OP_SUB },
      { VCODE_OP_SELECT },
      { VCODE_OP_CONST, .value = 1  },
      { VCODE_OP_ADD },
      { VCODE_OP_CAST },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_CMP, .cmp = VCODE_CMP_LT },
      { VCODE_OP_SELECT },
      { VCODE_OP_NEW },
      { VCODE_OP_ALL },
      { VCODE_OP_STORE, .name = "Y.mem" },
      { VCODE_OP_CONST, .value = 0 },
      { VCODE_OP_MEMSET },
      { VCODE_OP_WRAP },
      { VCODE_OP_STORE, .name = "Y" },
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_WAIT, .target = 1 }
   };

   CHECK_BB(0);

    EXPECT_BB(1) = {
       { VCODE_OP_INDEX, .name = "Y.mem" },
       { VCODE_OP_DEALLOCATE },
       { VCODE_OP_RETURN }
   };

   CHECK_BB(1);
}
END_TEST

START_TEST(test_mulphys)
{
   input_from_file(TESTDIR "/lower/mulphys.vhd");

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   tree_t e = run_elab();
   opt(e);
   lower_unit(e);

   vcode_unit_t v0 = tree_code(tree_decl(e, 1));
   vcode_select_unit(v0);

   EXPECT_BB(0) = {
      { VCODE_OP_CONST, .value = 1000000 },
      { VCODE_OP_CAST },
      { VCODE_OP_MUL },
      { VCODE_OP_RETURN }
   };

   CHECK_BB(0);
}
END_TEST

int main(void)
{
   term_init();

   Suite *s = suite_create("lower");

   TCase *tc = nvc_unit_test();
   tcase_add_test(tc, test_wait1);
   tcase_add_test(tc, test_assign1);
   tcase_add_test(tc, test_assign2);
   tcase_add_test(tc, test_signal1);
   tcase_add_test(tc, test_cond1);
   tcase_add_test(tc, test_arith1);
   tcase_add_test(tc, test_pack1);
   tcase_add_test(tc, test_func1);
   tcase_add_test(tc, test_issue94);
   tcase_add_test(tc, test_arrayop1);
   tcase_add_test(tc, test_array1);
   tcase_add_test(tc, test_nest1);
   tcase_add_test(tc, test_signal2);
   tcase_add_test(tc, test_attr1);
   tcase_add_test(tc, test_record1);
   tcase_add_test(tc, test_assign3);
   tcase_add_test(tc, test_signal4);
   tcase_add_test(tc, test_staticwait);
   tcase_add_test(tc, test_proc1);
   tcase_add_test(tc, test_while1);
   tcase_add_test(tc, test_loop1);
   tcase_add_test(tc, test_proc3);
   tcase_add_test(tc, test_loop2);
   tcase_add_test(tc, test_slice1);
   tcase_add_test(tc, test_funcif);
   tcase_add_test(tc, test_memset);
   tcase_add_test(tc, test_func5);
   tcase_add_test(tc, test_bounds1);
   tcase_add_test(tc, test_record6);
   tcase_add_test(tc, test_proc7);
   tcase_add_test(tc, test_mulphys);
   suite_add_tcase(s, tc);

   return nvc_run_test(s);
}
