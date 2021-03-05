/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZCORE_PP_SEQUENCES
#define AZCORE_PP_SEQUENCES

// Scripted bus event macros support up to 50 expansions.

#define AZ_SEQ_FOR_EACH(macro, seq) AZ_SEQ_JOIN(AZ_SEQ_FOR_EACH_, AZ_SEQ_COUNT(seq)(macro, seq))
#define AZ_SEQ_FOR_EACH_1(macro, seq)   macro(AZ_SEQ_HEAD(seq))
#define AZ_SEQ_FOR_EACH_2(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_1(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_3(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_2(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_4(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_3(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_5(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_4(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_6(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_5(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_7(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_6(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_8(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_7(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_9(macro, seq)   macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_8(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_10(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_9(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_11(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_10(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_12(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_11(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_13(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_12(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_14(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_13(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_15(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_14(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_16(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_15(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_17(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_16(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_18(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_17(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_19(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_18(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_20(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_19(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_21(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_20(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_22(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_21(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_23(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_22(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_24(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_23(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_25(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_24(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_26(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_25(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_27(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_26(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_28(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_27(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_29(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_28(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_30(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_29(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_31(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_30(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_32(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_31(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_33(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_32(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_34(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_33(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_35(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_34(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_36(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_35(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_37(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_36(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_38(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_37(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_39(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_38(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_40(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_39(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_41(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_40(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_42(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_41(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_43(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_42(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_44(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_43(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_45(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_44(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_46(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_45(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_47(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_46(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_48(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_47(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_49(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_48(macro, AZ_SEQ_TAIL(seq))
#define AZ_SEQ_FOR_EACH_50(macro, seq)  macro(AZ_SEQ_HEAD(seq)) AZ_SEQ_FOR_EACH_49(macro, AZ_SEQ_TAIL(seq))

#define AZ_SEQ_COUNT(seq)   AZ_SEQ_JOIN(AZ_SEQ_COUNT_0 seq, _END)
#define AZ_SEQ_COUNT_0(x)   AZ_SEQ_COUNT_1
#define AZ_SEQ_COUNT_1(x)   AZ_SEQ_COUNT_2
#define AZ_SEQ_COUNT_2(x)   AZ_SEQ_COUNT_3
#define AZ_SEQ_COUNT_3(x)   AZ_SEQ_COUNT_4
#define AZ_SEQ_COUNT_4(x)   AZ_SEQ_COUNT_5
#define AZ_SEQ_COUNT_5(x)   AZ_SEQ_COUNT_6
#define AZ_SEQ_COUNT_6(x)   AZ_SEQ_COUNT_7
#define AZ_SEQ_COUNT_7(x)   AZ_SEQ_COUNT_8
#define AZ_SEQ_COUNT_8(x)   AZ_SEQ_COUNT_9
#define AZ_SEQ_COUNT_9(x)   AZ_SEQ_COUNT_10
#define AZ_SEQ_COUNT_10(x)  AZ_SEQ_COUNT_11
#define AZ_SEQ_COUNT_11(x)  AZ_SEQ_COUNT_12
#define AZ_SEQ_COUNT_12(x)  AZ_SEQ_COUNT_13
#define AZ_SEQ_COUNT_13(x)  AZ_SEQ_COUNT_14
#define AZ_SEQ_COUNT_14(x)  AZ_SEQ_COUNT_15
#define AZ_SEQ_COUNT_15(x)  AZ_SEQ_COUNT_16
#define AZ_SEQ_COUNT_16(x)  AZ_SEQ_COUNT_17
#define AZ_SEQ_COUNT_17(x)  AZ_SEQ_COUNT_18
#define AZ_SEQ_COUNT_18(x)  AZ_SEQ_COUNT_19
#define AZ_SEQ_COUNT_19(x)  AZ_SEQ_COUNT_20
#define AZ_SEQ_COUNT_20(x)  AZ_SEQ_COUNT_21
#define AZ_SEQ_COUNT_21(x)  AZ_SEQ_COUNT_22
#define AZ_SEQ_COUNT_22(x)  AZ_SEQ_COUNT_23
#define AZ_SEQ_COUNT_23(x)  AZ_SEQ_COUNT_24
#define AZ_SEQ_COUNT_24(x)  AZ_SEQ_COUNT_25
#define AZ_SEQ_COUNT_25(x)  AZ_SEQ_COUNT_26
#define AZ_SEQ_COUNT_26(x)  AZ_SEQ_COUNT_27
#define AZ_SEQ_COUNT_27(x)  AZ_SEQ_COUNT_28
#define AZ_SEQ_COUNT_28(x)  AZ_SEQ_COUNT_29
#define AZ_SEQ_COUNT_29(x)  AZ_SEQ_COUNT_30
#define AZ_SEQ_COUNT_30(x)  AZ_SEQ_COUNT_31
#define AZ_SEQ_COUNT_31(x)  AZ_SEQ_COUNT_32
#define AZ_SEQ_COUNT_32(x)  AZ_SEQ_COUNT_33
#define AZ_SEQ_COUNT_33(x)  AZ_SEQ_COUNT_34
#define AZ_SEQ_COUNT_34(x)  AZ_SEQ_COUNT_35
#define AZ_SEQ_COUNT_35(x)  AZ_SEQ_COUNT_36
#define AZ_SEQ_COUNT_36(x)  AZ_SEQ_COUNT_37
#define AZ_SEQ_COUNT_37(x)  AZ_SEQ_COUNT_38
#define AZ_SEQ_COUNT_38(x)  AZ_SEQ_COUNT_39
#define AZ_SEQ_COUNT_39(x)  AZ_SEQ_COUNT_40
#define AZ_SEQ_COUNT_40(x)  AZ_SEQ_COUNT_41
#define AZ_SEQ_COUNT_41(x)  AZ_SEQ_COUNT_42
#define AZ_SEQ_COUNT_42(x)  AZ_SEQ_COUNT_43
#define AZ_SEQ_COUNT_43(x)  AZ_SEQ_COUNT_44
#define AZ_SEQ_COUNT_44(x)  AZ_SEQ_COUNT_45
#define AZ_SEQ_COUNT_45(x)  AZ_SEQ_COUNT_46
#define AZ_SEQ_COUNT_46(x)  AZ_SEQ_COUNT_47
#define AZ_SEQ_COUNT_47(x)  AZ_SEQ_COUNT_48
#define AZ_SEQ_COUNT_48(x)  AZ_SEQ_COUNT_49
#define AZ_SEQ_COUNT_49(x)  AZ_SEQ_COUNT_50
#define AZ_SEQ_COUNT_50(x)  AZ_SEQ_COUNT_ERROR

#define AZ_SEQ_COUNT_0_END  0
#define AZ_SEQ_COUNT_1_END  1
#define AZ_SEQ_COUNT_2_END  2
#define AZ_SEQ_COUNT_3_END  3
#define AZ_SEQ_COUNT_4_END  4
#define AZ_SEQ_COUNT_5_END  5
#define AZ_SEQ_COUNT_6_END  6
#define AZ_SEQ_COUNT_7_END  7
#define AZ_SEQ_COUNT_8_END  8
#define AZ_SEQ_COUNT_9_END  9
#define AZ_SEQ_COUNT_10_END 10
#define AZ_SEQ_COUNT_11_END 11
#define AZ_SEQ_COUNT_12_END 12
#define AZ_SEQ_COUNT_13_END 13
#define AZ_SEQ_COUNT_14_END 14
#define AZ_SEQ_COUNT_15_END 15
#define AZ_SEQ_COUNT_16_END 16
#define AZ_SEQ_COUNT_17_END 17
#define AZ_SEQ_COUNT_18_END 18
#define AZ_SEQ_COUNT_19_END 19
#define AZ_SEQ_COUNT_20_END 20
#define AZ_SEQ_COUNT_21_END 21
#define AZ_SEQ_COUNT_22_END 22
#define AZ_SEQ_COUNT_23_END 23
#define AZ_SEQ_COUNT_24_END 24
#define AZ_SEQ_COUNT_25_END 25
#define AZ_SEQ_COUNT_26_END 26
#define AZ_SEQ_COUNT_27_END 27
#define AZ_SEQ_COUNT_28_END 28
#define AZ_SEQ_COUNT_29_END 29
#define AZ_SEQ_COUNT_30_END 30
#define AZ_SEQ_COUNT_31_END 31
#define AZ_SEQ_COUNT_32_END 32
#define AZ_SEQ_COUNT_33_END 33
#define AZ_SEQ_COUNT_34_END 34
#define AZ_SEQ_COUNT_35_END 35
#define AZ_SEQ_COUNT_36_END 36
#define AZ_SEQ_COUNT_37_END 37
#define AZ_SEQ_COUNT_38_END 38
#define AZ_SEQ_COUNT_39_END 39
#define AZ_SEQ_COUNT_40_END 40
#define AZ_SEQ_COUNT_41_END 41
#define AZ_SEQ_COUNT_42_END 42
#define AZ_SEQ_COUNT_43_END 43
#define AZ_SEQ_COUNT_44_END 44
#define AZ_SEQ_COUNT_45_END 45
#define AZ_SEQ_COUNT_46_END 46
#define AZ_SEQ_COUNT_47_END 47
#define AZ_SEQ_COUNT_48_END 48
#define AZ_SEQ_COUNT_49_END 49
#define AZ_SEQ_COUNT_50_END 50

#define AZ_SEQ_HEAD(seq)        AZ_SEQ_HEAD_II((AZ_SEQ_HEAD_I seq))
#define AZ_SEQ_HEAD_I(x)        x, NIL
#define AZ_SEQ_HEAD_II(x)       AZ_SEQ_HEAD_IV(AZ_SEQ_HEAD_III x)
#define AZ_SEQ_HEAD_III(x, _)   x
#define AZ_SEQ_HEAD_IV(x)       x

#define AZ_SEQ_TAIL(seq)    AZ_SEQ_TAIL_I seq
#define AZ_SEQ_TAIL_I(x)

#define AZ_SEQ_JOIN(X, Y) AZ_SEQ_DO_JOIN(X, Y)
#define AZ_SEQ_DO_JOIN(X, Y) AZ_SEQ_DO_JOIN2(X, Y)
#define AZ_SEQ_DO_JOIN2(X, Y) X##Y

#define AZ_REP_APPLY0(X)
#define AZ_REP_APPLY1(X)  X(1)
#define AZ_REP_APPLY2(X)  AZ_REP_APPLY1(X)  X(2)
#define AZ_REP_APPLY3(X)  AZ_REP_APPLY2(X)  X(3)
#define AZ_REP_APPLY4(X)  AZ_REP_APPLY3(X)  X(4)
#define AZ_REP_APPLY5(X)  AZ_REP_APPLY4(X)  X(5)
#define AZ_REP_APPLY6(X)  AZ_REP_APPLY5(X)  X(6)
#define AZ_REP_APPLY7(X)  AZ_REP_APPLY6(X)  X(7)
#define AZ_REP_APPLY8(X)  AZ_REP_APPLY7(X)  X(8)
#define AZ_REP_APPLY9(X)  AZ_REP_APPLY8(X)  X(9)
#define AZ_REP_APPLY10(X) AZ_REP_APPLY9(X)  X(10)
#define AZ_REP_APPLY11(X) AZ_REP_APPLY10(X) X(11)
#define AZ_REP_APPLY12(X) AZ_REP_APPLY11(X) X(12)
#define AZ_REP_APPLY13(X) AZ_REP_APPLY12(X) X(13)
#define AZ_REP_APPLY14(X) AZ_REP_APPLY13(X) X(14)
#define AZ_REP_APPLY15(X) AZ_REP_APPLY14(X) X(15)
#define AZ_REP_APPLY16(X) AZ_REP_APPLY15(X) X(16)
#define AZ_REP_APPLY17(X) AZ_REP_APPLY16(X) X(17)
#define AZ_REP_APPLY18(X) AZ_REP_APPLY17(X) X(18)
#define AZ_REP_APPLY19(X) AZ_REP_APPLY18(X) X(19)
#define AZ_REP_APPLY20(X) AZ_REP_APPLY19(X) X(20)
#define AZ_REP_APPLY21(X) AZ_REP_APPLY20(X) X(21)
#define AZ_REP_APPLY22(X) AZ_REP_APPLY21(X) X(22)
#define AZ_REP_APPLY23(X) AZ_REP_APPLY22(X) X(23)
#define AZ_REP_APPLY24(X) AZ_REP_APPLY23(X) X(24)
#define AZ_REP_APPLY25(X) AZ_REP_APPLY24(X) X(25)
#define AZ_REP_APPLY26(X) AZ_REP_APPLY25(X) X(26)
#define AZ_REP_APPLY27(X) AZ_REP_APPLY26(X) X(27)
#define AZ_REP_APPLY28(X) AZ_REP_APPLY27(X) X(28)
#define AZ_REP_APPLY29(X) AZ_REP_APPLY28(X) X(29)
#define AZ_REP_APPLY30(X) AZ_REP_APPLY29(X) X(30)
#define AZ_REP_APPLY31(X) AZ_REP_APPLY30(X) X(31)
#define AZ_REP_APPLY32(X) AZ_REP_APPLY31(X) X(32)

#define AZ_PP_REPEAT(FUNC, N) AZ_PP_REPEAT_I(FUNC, N)
#define AZ_PP_REPEAT_I(FUNC, N) AZ_PP_REPEAT_II(AZ_SEQ_JOIN(AZ_REP_APPLY, N), FUNC)
#define AZ_PP_REPEAT_II(REPEATER, FUNC) REPEATER(FUNC)

#define AZ_DEC(x) AZ_DEC_IMPL(x)
#define AZ_DEC_IMPL(x) AZ_DEC_ ## x

#define AZ_DEC_1  0
#define AZ_DEC_2  1
#define AZ_DEC_3  2
#define AZ_DEC_4  3
#define AZ_DEC_5  4
#define AZ_DEC_6  5
#define AZ_DEC_7  6
#define AZ_DEC_8  7
#define AZ_DEC_9  8
#define AZ_DEC_10 9
#define AZ_DEC_11 10
#define AZ_DEC_12 11
#define AZ_DEC_13 12
#define AZ_DEC_14 13
#define AZ_DEC_15 14
#define AZ_DEC_16 15
#define AZ_DEC_17 16
#define AZ_DEC_18 17
#define AZ_DEC_19 18
#define AZ_DEC_20 19
#define AZ_DEC_21 20
#define AZ_DEC_22 21
#define AZ_DEC_23 22
#define AZ_DEC_24 23
#define AZ_DEC_25 24
#define AZ_DEC_26 25
#define AZ_DEC_27 26
#define AZ_DEC_28 27
#define AZ_DEC_29 28
#define AZ_DEC_30 29
#define AZ_DEC_31 30
#define AZ_DEC_32 31

#define AZ_INC(x) AZ_INC_IMPL(x)
#define AZ_INC_IMPL(x) AZ_INC_ ## x

#define AZ_INC_0  1
#define AZ_INC_1  2
#define AZ_INC_2  3
#define AZ_INC_3  4
#define AZ_INC_4  5
#define AZ_INC_5  6
#define AZ_INC_6  7
#define AZ_INC_7  8
#define AZ_INC_8  9
#define AZ_INC_9 10
#define AZ_INC_10 11
#define AZ_INC_11 12
#define AZ_INC_12 13
#define AZ_INC_13 14
#define AZ_INC_14 15
#define AZ_INC_15 16
#define AZ_INC_16 17
#define AZ_INC_17 18
#define AZ_INC_18 19
#define AZ_INC_19 20
#define AZ_INC_20 21
#define AZ_INC_21 22
#define AZ_INC_22 23
#define AZ_INC_23 24
#define AZ_INC_24 25
#define AZ_INC_25 26
#define AZ_INC_26 27
#define AZ_INC_27 28
#define AZ_INC_28 29
#define AZ_INC_29 30
#define AZ_INC_30 31
#define AZ_INC_31 32

#endif  // AZCORE_PP_SEQUENCES
