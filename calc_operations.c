
#include "calc_operations.h"
#include "basic_func.h"
#include "config.h"
#include "bi_def.h"
#include "const.h"

msg bi_add_ABc(OUT word* C, IN word* A, IN word* B, IN int c) {
    int c_out = 0;  // 반환용 carry(c')
    *C = *A + *B;
    c_out = (*C < *A) ? CARRY1 : CARRY0;     // if A + B < A, then c' = 1

    *C += c;
    c_out = (*C < c) ? c_out + 1 : c_out;   // if A + B + c < c, then c' += 1 

    return c_out;   // c' 반환
}

/* bi_add에서는 잘 되고, bi_textbook_mul에서는 안 되는 addc */
//msg bi_addc(OUT bigint** C, IN bigint** A, IN bigint** B) {
//    /* [n >= m] */
//    int n = (*A)->word_len;
//    int m = (*B)->word_len;
//
//
//    /* carry 연산하면서 A + B 수행 */
//
//    int c = 0;
//    for (int j = 0; j < n; j++) {
//        word b_value = (j < m) ? (*B)->a[j] : 0;
//        c = bi_add_ABc(&((*C)->a[j]), &((*A)->a[j]), &b_value, c);
//    }
//    if (c == CARRY1) {
//        (*C)->a[n] = CARRY1;
//    }
// 
//    return bi_refine(*C);
//}

msg bi_addc(OUT bigint** C, IN bigint** A, IN bigint** B) {
    /* [n >= m] */
    int n = (*A)->word_len;
    int m = (*B)->word_len;

    /* tmp 초기화 */
    bigint* tmp = NULL;
    bi_new(&tmp, n + 1);  // +1 워드는 최종 carry를 위한 공간

    /* carry 연산하면서 A + B 수행 */
    int c = 0;
    for (int j = 0; j < n; j++) {
        word a_value = (*A)->a[j];
        word b_value = (j < m) ? (*B)->a[j] : 0;

        c = bi_add_ABc(&(tmp->a[j]), &a_value, &b_value, c);
    }

    /* 마지막 carry 처리 */
    tmp->a[n] = (c == CARRY1) ? CARRY1 : CARRY0;

    /* C를 삭제하기 때문에 둘다 sign일 경우 음수로 지정 */
    tmp->sign = ((*A)->sign == NEGATIVE && (*B)->sign == NEGATIVE) ? NEGATIVE : NON_NEGATIVE;

    // 결과를 `C`에 복사
    bi_assign(C, tmp);

    // 메모리 해제
    bi_delete(&tmp);
    return bi_refine(*C);
}

msg bi_add(OUT bigint** C, IN bigint** A, IN bigint** B) {
    if (*A == NULL || *B == NULL) {
        printf("%s", SrcNULLErrMsg);
        return SrcNULLErr;
    }

    int n = (*A)->word_len;
    int m = (*B)->word_len;
    int max_word_len = (n > m) ? n + 1 : m + 1;
    
    bi_new(C, max_word_len);

    if ((*A)->sign != (*B)->sign) {
        // A > 0 and B < 0 
        if ((*A)->sign == NON_NEGATIVE) {
            (*B)->sign = NON_NEGATIVE;
            msg result = bi_sub(C, A, B);
            (*B)->sign = NEGATIVE;
            return result;
        }
        // A < 0 and B > 0 
        else {
            (*A)->sign = NON_NEGATIVE;
            msg result = bi_sub(C, B, A);
            (*A)->sign = NEGATIVE;
            return result;
        }
    }

    /* 부호가 같은 경우 */
    (*C)->sign = (*A)->sign;

    // wordlen A >= wordlen B
    if (n >= m) {
        return bi_addc(C, A, B);
    }
    else {
        return bi_addc(C, B, A);
    }

    return bi_refine(*C);
}

msg bi_sub_AbB(OUT word* C, IN word* A, IN int b, IN word* B) {
    int b_out = 0;  // 반환용 borrow(b')
    *C = *A - b;
    b_out = (*A < b) ? BORROW1 : BORROW0;    // if A < b, then b' = 1

    b_out = (*C < *B) ? b_out + 1 : b_out;   // if C < B, then b' += 1 
    *C -= *B;

    return b_out;   // b' 반환
}

msg bi_subc(OUT bigint** C, IN bigint** A, IN bigint** B) {
    /* [n >= m] */
    int n = (*A)->word_len;
    int m = (*B)->word_len;

    /* borrow 연산하면서 A - B 수행 */
    int b = 0;
    for (int j = 0; j < n; j++) {
        word a_value = (*A)->a[j];
        word b_value = (j < m) ? (*B)->a[j] : 0;
        b = bi_sub_AbB(&((*C)->a[j]), &a_value, b, &b_value);
    }

    return bi_refine(*C);
}

msg bi_sub(OUT bigint** C, IN bigint** A, IN bigint** B) {
    if (*A == NULL || *B == NULL) {
        printf("%s", SrcNULLErrMsg);
        return SrcNULLErr;
    }

    int n = (*A)->word_len;
    int m = (*B)->word_len;
    int max_word_len = (n > m) ? n + 1 : m + 1;

    bi_new(C, max_word_len);

    int sign_A = (*A)->sign;
    int sign_B = (*B)->sign;

    if (sign_A != sign_B) {
        // A > 0, B < 0인 경우 (A - (-B) = A + B)
        if (sign_A == NON_NEGATIVE) {
            (*B)->sign = NON_NEGATIVE;  // B를 양수로 변경
            msg result = bi_add(C, A, B);  // C = A + B
            (*B)->sign = sign_B;  // B의 부호를 원래대로 복원
            return result;
        }
        // A < 0, B > 0인 경우 (-A - B = -(A + B))
        else {
            (*A)->sign = NON_NEGATIVE;  // A를 양수로 변경
            msg result = bi_add(C, A, B);  // C = A + B
            (*A)->sign = sign_A;  // A의 부호를 원래대로 복원
            (*C)->sign = NEGATIVE;  // 결과 부호를 음수로 설정
            return result;
        }
    }

    /* 부호가 같은 경우 */
    // A > 0, B > 0인 경우
    if (sign_A == NON_NEGATIVE && sign_B == NON_NEGATIVE) {
        if (bi_compare(A, B) == COMPARE_GREATER) {
            return bi_subc(C, A, B);  // A > B일 때 C = A - B
        }
        else {
            // A < B인 경우 C = B - A, 결과 부호는 음수로 설정
            msg result = bi_subc(C, B, A);
            (*C)->sign = NEGATIVE;  // 결과 부호를 음수로 설정
            return result;
        }
    }

    // A < 0, B < 0인 경우
    if (sign_A == NEGATIVE && sign_B == NEGATIVE) {
        if (bi_compare(A, B) == COMPARE_GREATER) {
            // A > B일 경우 C = B - A, 결과 부호는 양수로 설정
            return bi_subc(C, B, A);
        }
        else {
            // A < B일 경우 C = A - B, 결과 부호는 음수로 설정
            msg result = bi_subc(C, A, B);
            (*C)->sign = NEGATIVE;  // 결과 부호를 음수로 설정
            return result;
        }
    }

    return bi_refine(*C);
}

msg bi_mul_AB(OUT word C[2], IN word* A, IN word* B) {
    /* 하나의 워드를 두 개로 나눔 */
    /* 상위비트(A1, B1)와 하위비트(A0, B0)로 나눔 */
    word A1 = *A >> (WORD_BITLEN / 2);
    word A0 = *A & WORD_MASK;
    word B1 = *B >> (WORD_BITLEN / 2);
    word B0 = *B & WORD_MASK;

    /* A1B0 + A0B1 = A1B0 + A0B1 + carry */
    word T1 = A1 * B0;
    word T0 = A0 * B1;

    T0 = T1 + T0;
    T1 = (T0 < T1);

    /* A1B1, A0B0 */
    word C1 = A1 * B1;
    word C0 = A0 * B0;

    word T = C0;

    /* C0: 두 개의 워드에서 하위 비트 */
    /* C1: 두 개의 워드에서 상위 비트 */
    C0 = C0 + (T0 << (WORD_BITLEN / 2));
    C1 = C1 + (T1 << (WORD_BITLEN / 2)) + (T0 >> (WORD_BITLEN / 2)) + (C0 < T);

    C[1] = C1;
    C[0] = C0;

    return CLEAR;
}

msg bi_textbook_mulc(OUT bigint** C, IN bigint** A, IN bigint** B) {
    int n = (*A)->word_len;
    int m = (*B)->word_len;

    msg result;

    /* 워드 길이 2로 초기화 */
    bi_new(C, 2);

    /* Aj*Bi 단일 워드 곱셈의 합 */
    for (int Aj = 0; Aj < n; Aj++) {
        for (int Bi = 0; Bi < m; Bi++) {
            /* 단일 워드 곱셈 저장 */
            bigint* T = NULL;
            result = bi_new(&T, 2);
            result = bi_mul_AB(T->a, &((*A)->a[Aj]), &((*B)->a[Bi]));

            /* Aj + Bi만큼 left shift */
            bi_word_shift_left(&T, Aj + Bi);

            /* 2 + Aj + Bi에 carry 발생을 고려하여 C의 워드 길이 1 증가 */
            int required_len = 2 + Aj + Bi + CARRY1;
            if ((*C)->word_len < required_len) {
                // bigint의 배열 확장
                word* temp = (word*)realloc((*C)->a, required_len * sizeof(word));
                if (temp == NULL) {
                    fprintf(stderr, MemAllocErrMsg);
                    return MemAllocErr;
                }
                (*C)->a = temp;

                /* 새로 확장된 부분을 0으로 초기화 */
                memset((*C)->a + (*C)->word_len, 0, (required_len - (*C)->word_len) * sizeof(word));
                (*C)->word_len = required_len;
            }

            /* C = C + T */
            result = bi_addc(C, C, &T);

            bi_delete(&T);
        }
    }
    return bi_refine(*C);
}

msg bi_karatsuba_mulc(OUT bigint** C, IN bigint** A, IN bigint** B) {
    int n = (*A)->word_len;
    int m = (*B)->word_len;

    /* 기본 처리: 곱할 숫자 중 하나가 0인 경우 */
    if (n == 0 || m == 0) {
        bi_new(C, 1);
        (*C)->a[0] = 0;
        return CLEAR;
    }

    /* 작은 입력 크기의 경우 기본 곱셈 호출 */
    if (n <= FLAG || m <= FLAG) {
        return bi_textbook_mulc(C, A, B);  // 교재식 곱셈
    }

    /* A와 B의 상/하위 계산 */
    int max_len = (n > m ? n : m);  // 최대 길이
    int half_len = max_len / 2;     // 절반 길이

    /* A_low, A_high를 직접 처리 */
    bigint* A_low = NULL, * A_high = NULL;
    bi_new(&A_low, half_len);
    bi_new(&A_high, n > half_len ? n - half_len : 0);

    memcpy(A_low->a, (*A)->a, sizeof(word) * half_len);  // 하위 복사
    if (A_high->word_len > 0) {
        memcpy(A_high->a, (*A)->a + half_len, sizeof(word) * A_high->word_len);  // 상위 복사
    }

    /* B_low, B_high를 직접 처리 */
    bigint* B_low = NULL, * B_high = NULL;
    bi_new(&B_low, half_len);
    bi_new(&B_high, m > half_len ? m - half_len : 0);

    memcpy(B_low->a, (*B)->a, sizeof(word) * half_len);  // 하위 복사
    if (B_high->word_len > 0) {
        memcpy(B_high->a, (*B)->a + half_len, sizeof(word) * B_high->word_len);  // 상위 복사
    }

    /* Z2 = A_high * B_high */
    bigint* Z2 = NULL;
    bi_karatsuba_mulc(&Z2, &A_high, &B_high);

    /* Z0 = A_low * B_low */
    bigint* Z0 = NULL;
    bi_karatsuba_mulc(&Z0, &A_low, &B_low);

    /* Z1 = (A_high + A_low) * (B_high + B_low) - Z2 - Z0 */
    bigint* A_sum = NULL, * B_sum = NULL, * Z1 = NULL;
    bi_add(&A_sum, &A_high, &A_low);  // A_high + A_low
    bi_add(&B_sum, &B_high, &B_low);  // B_high + B_low
    bi_karatsuba_mulc(&Z1, &A_sum, &B_sum);

    bi_subc(&Z1, &Z1, &Z2);  // Z1 -= Z2
    bi_subc(&Z1, &Z1, &Z0);  // Z1 -= Z0

    /* 결과 합산: Z2 << (2 * half_len), Z1 << half_len, Z0 */
    bi_word_shift_left(&Z2, 2 * half_len);  // Z2를 2배 쉬프트
    bi_word_shift_left(&Z1, half_len);      // Z1을 1배 쉬프트

    bi_addc(C, &Z2, &Z1);  // C = Z2 + Z1
    bi_addc(C, C, &Z0);    // C += Z0

    /* 메모리 정리 */
    bi_delete(&A_low);
    bi_delete(&A_high);
    bi_delete(&B_low);
    bi_delete(&B_high);
    bi_delete(&Z2);
    bi_delete(&Z0);
    bi_delete(&Z1);
    bi_delete(&A_sum);
    bi_delete(&B_sum);

    return bi_refine(*C);
}

msg bi_mul(OUT bigint** C, IN bigint** A, IN bigint** B) {
    bi_karatsuba_mulc(C, A, B);
    (*C)->sign = (((*A)->sign ^ (*B)->sign) == 0) ? NON_NEGATIVE : NEGATIVE;

    return CLEAR;
}


msg bi_long_div(OUT bigint** Q, OUT bigint** R, IN bigint** A, IN bigint** B) {
    /* bigint NULL 체크 */
    if (*A == NULL || *B == NULL) {
        fprintf(stderr, SrcNULLErrMsg);
        return SrcNULLErr;
    }

    /* Zero Divisor 체크 */
    if (((*B)->word_len == 1) && ((*B)->a[0] == 0x0)) {
        fprintf(stderr, ZeroDivisorErrMsg);
        return ZeroDivisorErr;
    }

    int n = (*A)->word_len;
    int m = (*B)->word_len;

    /* Q와 R 초기화 */
    bi_new(Q, n);
    bi_new(R, n);

    /* 비트 단위 나눗셈 */
    for (int j = (n * WORD_BITLEN) - 1; j >= 0; j--) {

        /* R = 2R */
        bi_doubling(*R);

        /* R ^= Aj */
        if (((*A)->a[j / WORD_BITLEN] & ((word)1 << (j % WORD_BITLEN))) != 0) {
            (*R)->a[0] ^= 1;
        }

        /* R >= B 체크 */
        bigint* tmpB = NULL;
        bigint* tmpR = NULL;
        bi_assign(&tmpB, *B);
        bi_assign(&tmpR, *R);

        bi_refine(tmpB);
        bi_refine(tmpR);

        if (bi_compareABS(&tmpR, &tmpB) != COMPARE_LESS) {
            /* Q = Q + (1 << j) */
            (*Q)->a[j / WORD_BITLEN] ^= ((word)1 << (j % WORD_BITLEN));

            /* R - B */
            bi_subc(R, R, B);
        }
        bi_delete(&tmpB);
        bi_delete(&tmpR);
    }

    bi_refine(*Q);
    bi_refine(*R);
    
    return CLEAR;
}

msg bi_div(OUT bigint** Q, OUT bigint** R, IN bigint** A, IN bigint** B) {
    /* bigint NULL 체크 */
    if (*A == NULL || *B == NULL) {
        fprintf(stderr, SrcNULLErrMsg);
        return SrcNULLErr;
    }

    /* Zero Divisor 체크 */
    if (((*B)->word_len == 1) && ((*B)->a[0] == 0x0)) {
        fprintf(stderr, ZeroDivisorErrMsg);
        return ZeroDivisorErr;
    }
    int sign_A = (*A)->sign;
    int sign_B = (*B)->sign;

    (*A)->sign = NON_NEGATIVE;  // A 절댓값
    (*B)->sign = NON_NEGATIVE;  // B 절댓값
    bi_long_div(Q, R, A, B);  // 절댓값 기준 나눗셈 수행


    /* R이 B보다 크거나 같은 경우 보정 */
    if (sign_A == NEGATIVE) {
        bigint* one = NULL;
        bi_set_from_string(&one, "1", 2);  // 1 생성
        bi_addc(Q, Q, &one);
        bi_delete(&one);

        bigint* new_R = NULL;
        bi_sub(&new_R, B, R);
        bi_assign(R, new_R);  // R 업데이트
        bi_delete(&new_R);
    }


    (*Q)->sign = (sign_A == sign_B) ? NON_NEGATIVE : NEGATIVE;
    (*R)->sign = NON_NEGATIVE;   // R은 항상 양수
    (*A)->sign = sign_A;  // A 절댓값
    (*B)->sign = sign_B;  // B 절댓값

    return CLEAR;
}
