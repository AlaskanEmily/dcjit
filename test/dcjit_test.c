#include "dcjit_test.h"
#include "dc.h"

static const float dc_epsilon = 0.00001f;

#define COMMA ,

/* Expects a calculation to succeed. */
static int run_calculation(const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const float *args,
    float *out){
    const char *err;
    struct DC_Context *const ctx = DC_CreateContext();
    struct DC_Calculation *const calc =
        DC_CompileCalculation(ctx, source, num_args, arg_names, &err);
    
    if(err == NULL)
        YYY_ASSERT_TRUE(calc != NULL);
    if(calc == NULL)
        YYY_ASSERT_TRUE(err != NULL);
    YYY_ASSERT_TRUE((err == NULL) != (calc == NULL));
    if(err != NULL){
        YYY_ERR_PRINTF("Error compiling calculation \"%s\": %s\n",
            source,
            err);
        DC_FreeError(err);
        DC_FreeContext(ctx);
        return 0;
    }
    else{
        out[0] = DC_Calculate(calc, args);
        DC_Free(ctx, calc);
        DC_FreeContext(ctx);
        return 1;
    }
}

/* Expects a calculation to fail. */
static int fail_calculation(const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const float *args){
    const char *err;
    struct DC_Context *const ctx = DC_CreateContext();
    struct DC_Calculation *const calc =
        DC_CompileCalculation(ctx, source, num_args, arg_names, &err);

    if(err == NULL)
        YYY_ASSERT_TRUE(calc != NULL);
    if(calc == NULL)
        YYY_ASSERT_TRUE(err != NULL);
    YYY_ASSERT_TRUE((err == NULL) != (calc == NULL));
    if(err == NULL){
        YYY_ERR_PRINTF("Expected error in calculation \"%s\"\n", source);
        DC_Free(ctx, calc);
        DC_FreeContext(ctx);
        return 0;
    }
    else{
        DC_FreeError(err);
        DC_FreeContext(ctx);
        return 1;
    }
}

#define RUN_CALCULATION_ARGS(SOURCE, ARGNAMES, ARGS, VALUE) do{\
        const char *const argnames[] = ARGNAMES;\
        const float args[] = ARGS;\
        const unsigned num_args = sizeof(args)/sizeof(float);\
        YYY_ASSERT_INT_EQ(sizeof(argnames)/sizeof(void*), num_args);\
        YYY_ASSERT_TRUE(run_calculation((SOURCE), num_args, argnames, args, (VALUE)));\
    }while(0)

#define RUN_CALCULATION(SOURCE, VALUE) do{\
        YYY_ASSERT_TRUE(run_calculation((SOURCE), 0, NULL, NULL, (VALUE)));\
    }while(0)

/* Tests that "0" evaluates to 0.0" */
static int zero_immediate_test(void){
    float value;
    RUN_CALCULATION("0", &value);
    /* Expect an exact match for zero. */
    YYY_ASSERT_FLOAT_EQ(value, 0.0f, 0.0f);
    return 1;
}

/* Tests that "1" evaluates to 1.0" */
static int one_immediate_test(void){
    float value;
    RUN_CALCULATION("1", &value);
    /* Expect an exact match for one. */
    YYY_ASSERT_FLOAT_EQ(value, 1.0f, 0.0f);
    return 1;
}

/* Tests that "0.0" evaluates to 0.0" */
static int zero_decimal_immediate_test(void){
    float value;
    RUN_CALCULATION("0.000", &value);
    /* Expect an exact match for zero. */
    YYY_ASSERT_FLOAT_EQ(value, 0.0f, 0.0f);
    return 1;
}

/* Tests that "1.0" evaluates to 1.0" */
static int one_decimal_immediate_test(void){
    float value;
    RUN_CALCULATION("1.000", &value);
    /* Expect an exact match for one. */
    YYY_ASSERT_FLOAT_EQ(value, 1.0f, 0.0f);
    return 1;
}

/* Tests that evaluating the first (index zero) arg works. */
static int zero_arg_test(void){
    float value;
    RUN_CALCULATION_ARGS("x", {"x"}, {23.987f}, &value);
    /* Expect an exact match for one. */
    YYY_ASSERT_FLOAT_EQ(value, 23.987f, dc_epsilon);
    return 1;
}

/* Tests that evaluating the first (index zero) of multiple args works. */
static int zero_of_two_arg_test(void){
    float value;
    RUN_CALCULATION_ARGS("x", {"x" COMMA "y"}, {23.987f COMMA 0.999f}, &value);
    /* Expect an exact match for one. */
    YYY_ASSERT_FLOAT_EQ(value, 23.987f, dc_epsilon);
    return 1;
}

/* Tests that evaluating the second (index one) arg works. */
static int one_arg_test(void){
    float value;
    RUN_CALCULATION_ARGS("x", {"y" COMMA "x"}, {99.0 COMMA 23.987f}, &value);
    /* Expect an exact match for one. */
    YYY_ASSERT_FLOAT_EQ(value, 23.987f, dc_epsilon);
    return 1;
}

static struct YYY_Test dc_test_tests[] = {
    YYY_TEST(zero_immediate_test),
    YYY_TEST(one_immediate_test),
    YYY_TEST(zero_decimal_immediate_test),
    YYY_TEST(one_decimal_immediate_test),
    YYY_TEST(zero_arg_test),
    YYY_TEST(zero_of_two_arg_test),
    YYY_TEST(one_arg_test),
};

YYY_TEST_FUNCTION(DC_Test_RunTests, dc_test_tests, "DCJIT")

int main(int argc, char **argv){
    (void)argc;
    (void)argv;
    DC_Test_RunTests();
}
