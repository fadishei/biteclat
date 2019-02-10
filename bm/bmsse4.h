#ifndef BMSSE4__H__INCLUDED__
#define BMSSE4__H__INCLUDED__
/*
Copyright(c) 2002-2017 Anatoliy Kuznetsov(anatoliy_kuznetsov at yahoo.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For more information please visit:  http://bitmagic.io
*/

/*! \file bmsse4.h
    \brief Compute functions for SSE4.2 SIMD instruction set (internal)
*/

#include<mmintrin.h>
#include<emmintrin.h>
#include<smmintrin.h>
#include<nmmintrin.h>
#include<immintrin.h>

#include "bmdef.h"
#include "bmsse_util.h"
#include "bmutil.h"

namespace bm
{

/** @defgroup SSE4 SSE4.2 funcions (internal)
    Processor specific optimizations for SSE4.2 instructions (internals)
    @internal
    @ingroup bvector
 */

#ifdef __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

/*
inline
void sse2_print128(const char* prefix, const __m128i & value)
{
    const size_t n = sizeof(__m128i) / sizeof(unsigned);
    unsigned buffer[n];
    _mm_storeu_si128((__m128i*)buffer, value);
    std::cout << prefix << " [ ";
    for (int i = n-1; 1; --i)
    {
        std::cout << buffer[i] << " ";
        if (i == 0)
            break;
    }
    std::cout << "]" << std::endl;
}
*/

/*!
    SSE4.2 optimized bitcounting .
    @ingroup SSE4
*/
inline 
bm::id_t sse4_bit_count(const __m128i* block, const __m128i* block_end)
{
    bm::id_t count = 0;
#ifdef BM64_SSE4
    const bm::id64_t* b = (bm::id64_t*) block;
    const bm::id64_t* b_end = (bm::id64_t*) block_end;
    do
    {
        count += unsigned( _mm_popcnt_u64(b[0]) +
                           _mm_popcnt_u64(b[1]));
        b += 2;
    } while (b < b_end);
#else
    do
    {
        const unsigned* b = (unsigned*) block;
        count += _mm_popcnt_u32(b[0]) +
                 _mm_popcnt_u32(b[1]) +
                 _mm_popcnt_u32(b[2]) +
                 _mm_popcnt_u32(b[3]);
    } while (++block < block_end);
#endif    
    return count;
}

/*!
\internal
*/
BMFORCEINLINE 
unsigned op_xor(unsigned a, unsigned b)
{
    unsigned ret = (a ^ b);
    return ret;
}

/*!
\internal
*/
BMFORCEINLINE 
unsigned op_or(unsigned a, unsigned b)
{
    return (a | b);
}

/*!
\internal
*/
BMFORCEINLINE 
unsigned op_and(unsigned a, unsigned b)
{
    return (a & b);
}


template<class Func>
bm::id_t sse4_bit_count_op(const __m128i* BMRESTRICT block, 
                           const __m128i* BMRESTRICT block_end,
                           const __m128i* BMRESTRICT mask_block,
                           Func sse2_func)
{
    bm::id_t count = 0;
#ifdef BM64_SSE4
    do
    {
        __m128i tmp0 = _mm_load_si128(block);
        __m128i tmp1 = _mm_load_si128(mask_block);        
        __m128i b = sse2_func(tmp0, tmp1);

        count += (unsigned)_mm_popcnt_u64(_mm_extract_epi64(b, 0));
        count += (unsigned)_mm_popcnt_u64(_mm_extract_epi64(b, 1));

        ++block; ++mask_block;
    } while (block < block_end);
#else    
    do
    {
        __m128i tmp0 = _mm_load_si128(block);
        __m128i tmp1 = _mm_load_si128(mask_block);        
        __m128i b = sse2_func(tmp0, tmp1);

        count += _mm_popcnt_u32(_mm_extract_epi32(b, 0));
        count += _mm_popcnt_u32(_mm_extract_epi32(b, 1));
        count += _mm_popcnt_u32(_mm_extract_epi32(b, 2));
        count += _mm_popcnt_u32(_mm_extract_epi32(b, 3));

        ++block; ++mask_block;
    } while (block < block_end);
#endif
    
    return count;
}

/*!
    @brief check if block is all zero bits
    @ingroup SSE4
*/
inline
bool sse4_is_all_zero(const __m128i* BMRESTRICT block)
{
    __m128i w;
    __m128i maskz = _mm_setzero_si128();
    const __m128i* BMRESTRICT block_end =
        (const __m128i*)((bm::word_t*)(block) + bm::set_block_size);

    do
    {
        w = _mm_or_si128(_mm_load_si128(block+0), _mm_load_si128(block+1));
        if (!_mm_test_all_ones(_mm_cmpeq_epi8(w, maskz))) // (w0 | w1) != maskz
            return false;
        w = _mm_or_si128(_mm_load_si128(block+2), _mm_load_si128(block+3));
        if (!_mm_test_all_ones(_mm_cmpeq_epi8(w, maskz))) // (w0 | w1) != maskz
            return false;
        block += 4;
    } while (block < block_end);
    return true;
}

/*!
    @brief check if digest stride is all zero bits
    @ingroup SSE4
*/
inline
bool sse4_is_digest_zero(const __m128i* BMRESTRICT block)
{
    __m128i wA = _mm_or_si128(_mm_load_si128(block+0), _mm_load_si128(block+1));
    __m128i wB = _mm_or_si128(_mm_load_si128(block+2), _mm_load_si128(block+3));
    wA = _mm_or_si128(wA, wB);
    bool z1 = _mm_test_all_zeros(wA, wA);

    wA = _mm_or_si128(_mm_load_si128(block+4), _mm_load_si128(block+5));
    wB = _mm_or_si128(_mm_load_si128(block+6), _mm_load_si128(block+7));
    wA = _mm_or_si128(wA, wB);
    bool z2 = _mm_test_all_zeros(wA, wA);
    return z1 & z2;
}


/*!
    @brief AND blocks2
    *dst &= *src
 
    @return 0 if no bits were set

    @ingroup SSE4
*/
inline
unsigned sse4_and_block(__m128i* BMRESTRICT dst,
                       const __m128i* BMRESTRICT src)
{
    __m128i m1A, m1B, m1C, m1D;
    __m128i accA, accB, accC, accD;
    
    const __m128i* BMRESTRICT src_end =
        (const __m128i*)((bm::word_t*)(src) + bm::set_block_size);

    accA = accB = accC = accD = _mm_setzero_si128();

    do
    {
        m1A = _mm_and_si128(_mm_load_si128(src+0), _mm_load_si128(dst+0));
        m1B = _mm_and_si128(_mm_load_si128(src+1), _mm_load_si128(dst+1));
        m1C = _mm_and_si128(_mm_load_si128(src+2), _mm_load_si128(dst+2));
        m1D = _mm_and_si128(_mm_load_si128(src+3), _mm_load_si128(dst+3));

        _mm_store_si128(dst+0, m1A);
        _mm_store_si128(dst+1, m1B);
        _mm_store_si128(dst+2, m1C);
        _mm_store_si128(dst+3, m1D);

        accA = _mm_or_si128(accA, m1A);
        accB = _mm_or_si128(accB, m1B);
        accC = _mm_or_si128(accC, m1C);
        accD = _mm_or_si128(accD, m1D);
        
        src += 4; dst += 4;
    } while (src < src_end);
    
    accA = _mm_or_si128(accA, accB); // A = A | B
    accC = _mm_or_si128(accC, accD); // C = C | D
    accA = _mm_or_si128(accA, accC); // A = A | C
    
    return !_mm_testz_si128(accA, accA);
}


/*!
    @brief AND block digest stride
    *dst &= *src
 
    @return true if stide is all zero
    @ingroup SSE4
*/
inline
bool sse4_and_digest(__m128i* BMRESTRICT dst,
                     const __m128i* BMRESTRICT src)
{
    __m128i m1A, m1B, m1C, m1D;

    m1A = _mm_and_si128(_mm_load_si128(src+0), _mm_load_si128(dst+0));
    m1B = _mm_and_si128(_mm_load_si128(src+1), _mm_load_si128(dst+1));
    m1C = _mm_and_si128(_mm_load_si128(src+2), _mm_load_si128(dst+2));
    m1D = _mm_and_si128(_mm_load_si128(src+3), _mm_load_si128(dst+3));

    _mm_store_si128(dst+0, m1A);
    _mm_store_si128(dst+1, m1B);
    _mm_store_si128(dst+2, m1C);
    _mm_store_si128(dst+3, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z1 = _mm_testz_si128(m1A, m1A);
    
    m1A = _mm_and_si128(_mm_load_si128(src+4), _mm_load_si128(dst+4));
    m1B = _mm_and_si128(_mm_load_si128(src+5), _mm_load_si128(dst+5));
    m1C = _mm_and_si128(_mm_load_si128(src+6), _mm_load_si128(dst+6));
    m1D = _mm_and_si128(_mm_load_si128(src+7), _mm_load_si128(dst+7));

    _mm_store_si128(dst+4, m1A);
    _mm_store_si128(dst+5, m1B);
    _mm_store_si128(dst+6, m1C);
    _mm_store_si128(dst+7, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z2 = _mm_testz_si128(m1A, m1A);
    
     return z1 & z2;
}

/*!
    @brief AND block digest stride
    *dst = *src1 & src2
 
    @return true if stide is all zero
    @ingroup SSE4
*/
inline
bool sse4_and_digest_2way(__m128i* BMRESTRICT dst,
                          const __m128i* BMRESTRICT src1,
                          const __m128i* BMRESTRICT src2)
{
    __m128i m1A, m1B, m1C, m1D;

    m1A = _mm_and_si128(_mm_load_si128(src1+0), _mm_load_si128(src2+0));
    m1B = _mm_and_si128(_mm_load_si128(src1+1), _mm_load_si128(src2+1));
    m1C = _mm_and_si128(_mm_load_si128(src1+2), _mm_load_si128(src2+2));
    m1D = _mm_and_si128(_mm_load_si128(src1+3), _mm_load_si128(src2+3));
    
    _mm_store_si128(dst+0, m1A);
    _mm_store_si128(dst+1, m1B);
    _mm_store_si128(dst+2, m1C);
    _mm_store_si128(dst+3, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z1 = _mm_testz_si128(m1A, m1A);
    
    m1A = _mm_and_si128(_mm_load_si128(src1+4), _mm_load_si128(src2+4));
    m1B = _mm_and_si128(_mm_load_si128(src1+5), _mm_load_si128(src2+5));
    m1C = _mm_and_si128(_mm_load_si128(src1+6), _mm_load_si128(src2+6));
    m1D = _mm_and_si128(_mm_load_si128(src1+7), _mm_load_si128(src2+7));

    _mm_store_si128(dst+4, m1A);
    _mm_store_si128(dst+5, m1B);
    _mm_store_si128(dst+6, m1C);
    _mm_store_si128(dst+7, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z2 = _mm_testz_si128(m1A, m1A);
    
     return z1 & z2;
}

/*!
    @brief AND block digest stride
    @return true if stide is all zero
    @ingroup SSE4
*/
inline
bool sse4_and_digest_5way(__m128i* BMRESTRICT dst,
                          const __m128i* BMRESTRICT src1,
                          const __m128i* BMRESTRICT src2,
                          const __m128i* BMRESTRICT src3,
                          const __m128i* BMRESTRICT src4)
{
    __m128i m1A, m1B, m1C, m1D;
    __m128i m1E, m1F, m1G, m1H;

    m1A = _mm_and_si128(_mm_load_si128(src1+0), _mm_load_si128(src2+0));
    m1B = _mm_and_si128(_mm_load_si128(src1+1), _mm_load_si128(src2+1));
    m1C = _mm_and_si128(_mm_load_si128(src1+2), _mm_load_si128(src2+2));
    m1D = _mm_and_si128(_mm_load_si128(src1+3), _mm_load_si128(src2+3));

    m1E = _mm_and_si128(_mm_load_si128(src3+0), _mm_load_si128(src4+0));
    m1F = _mm_and_si128(_mm_load_si128(src3+1), _mm_load_si128(src4+1));
    m1G = _mm_and_si128(_mm_load_si128(src3+2), _mm_load_si128(src4+2));
    m1H = _mm_and_si128(_mm_load_si128(src3+3), _mm_load_si128(src4+3));

    m1A = _mm_and_si128(m1A, m1E);
    m1B = _mm_and_si128(m1B, m1F);
    m1C = _mm_and_si128(m1C, m1G);
    m1D = _mm_and_si128(m1D, m1H);

    m1A = _mm_and_si128(m1A, _mm_load_si128(dst+0));
    m1B = _mm_and_si128(m1B, _mm_load_si128(dst+1));
    m1C = _mm_and_si128(m1C, _mm_load_si128(dst+2));
    m1D = _mm_and_si128(m1D, _mm_load_si128(dst+3));

    _mm_store_si128(dst+0, m1A);
    _mm_store_si128(dst+1, m1B);
    _mm_store_si128(dst+2, m1C);
    _mm_store_si128(dst+3, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z1 = _mm_testz_si128(m1A, m1A);
    
    m1A = _mm_and_si128(_mm_load_si128(src1+4), _mm_load_si128(src2+4));
    m1B = _mm_and_si128(_mm_load_si128(src1+5), _mm_load_si128(src2+5));
    m1C = _mm_and_si128(_mm_load_si128(src1+6), _mm_load_si128(src2+6));
    m1D = _mm_and_si128(_mm_load_si128(src1+7), _mm_load_si128(src2+7));

    m1E = _mm_and_si128(_mm_load_si128(src3+4), _mm_load_si128(src4+4));
    m1F = _mm_and_si128(_mm_load_si128(src3+5), _mm_load_si128(src4+5));
    m1G = _mm_and_si128(_mm_load_si128(src3+6), _mm_load_si128(src4+6));
    m1H = _mm_and_si128(_mm_load_si128(src3+7), _mm_load_si128(src4+7));

    m1A = _mm_and_si128(m1A, m1E);
    m1B = _mm_and_si128(m1B, m1F);
    m1C = _mm_and_si128(m1C, m1G);
    m1D = _mm_and_si128(m1D, m1H);

    m1A = _mm_and_si128(m1A, _mm_load_si128(dst+4));
    m1B = _mm_and_si128(m1B, _mm_load_si128(dst+5));
    m1C = _mm_and_si128(m1C, _mm_load_si128(dst+6));
    m1D = _mm_and_si128(m1D, _mm_load_si128(dst+7));

    _mm_store_si128(dst+4, m1A);
    _mm_store_si128(dst+5, m1B);
    _mm_store_si128(dst+6, m1C);
    _mm_store_si128(dst+7, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z2 = _mm_testz_si128(m1A, m1A);
    
     return z1 & z2;
}


/*!
    @brief SUB (AND NOT) block digest stride
    *dst &= ~*src
 
    @return true if stide is all zero
    @ingroup SSE4
*/
inline
bool sse4_sub_digest(__m128i* BMRESTRICT dst,
                     const __m128i* BMRESTRICT src)
{
    __m128i m1A, m1B, m1C, m1D;

    m1A = _mm_andnot_si128(_mm_load_si128(src+0), _mm_load_si128(dst+0));
    m1B = _mm_andnot_si128(_mm_load_si128(src+1), _mm_load_si128(dst+1));
    m1C = _mm_andnot_si128(_mm_load_si128(src+2), _mm_load_si128(dst+2));
    m1D = _mm_andnot_si128(_mm_load_si128(src+3), _mm_load_si128(dst+3));

    _mm_store_si128(dst+0, m1A);
    _mm_store_si128(dst+1, m1B);
    _mm_store_si128(dst+2, m1C);
    _mm_store_si128(dst+3, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z1 = _mm_testz_si128(m1A, m1A);
    
    m1A = _mm_andnot_si128(_mm_load_si128(src+4), _mm_load_si128(dst+4));
    m1B = _mm_andnot_si128(_mm_load_si128(src+5), _mm_load_si128(dst+5));
    m1C = _mm_andnot_si128(_mm_load_si128(src+6), _mm_load_si128(dst+6));
    m1D = _mm_andnot_si128(_mm_load_si128(src+7), _mm_load_si128(dst+7));

    _mm_store_si128(dst+4, m1A);
    _mm_store_si128(dst+5, m1B);
    _mm_store_si128(dst+6, m1C);
    _mm_store_si128(dst+7, m1D);
    
     m1A = _mm_or_si128(m1A, m1B);
     m1C = _mm_or_si128(m1C, m1D);
     m1A = _mm_or_si128(m1A, m1C);
    
     bool z2 = _mm_testz_si128(m1A, m1A);
    
     return z1 & z2;
}



/*!
    @brief check if block is all zero bits
    @ingroup SSE4
*/
inline
bool sse4_is_all_one(const __m128i* BMRESTRICT block)
{
    __m128i w;
    const __m128i* BMRESTRICT block_end =
        (const __m128i*)((bm::word_t*)(block) + bm::set_block_size);

    do
    {
        w = _mm_and_si128(_mm_load_si128(block+0), _mm_load_si128(block+1));
        if (!_mm_test_all_ones(w))
            return false;
        w = _mm_and_si128(_mm_load_si128(block+2), _mm_load_si128(block+3));
        if (!_mm_test_all_ones(w))
            return false;

        block+=4;
    } while (block < block_end);
    return true;
}

/*!
    @brief check if wave of pointers is all NULL
    @ingroup SSE4
*/
BMFORCEINLINE
bool sse42_test_all_zero_wave(const void* ptr)
{
    __m128i w0 = _mm_loadu_si128((__m128i*)ptr);
    return _mm_testz_si128(w0, w0);
}

/*!
    SSE4.2 calculate number of bit changes from 0 to 1
    @ingroup SSE4
*/
inline
unsigned sse42_bit_block_calc_change(const __m128i* BMRESTRICT block)
{
    const __m128i* block_end =
        ( __m128i*)((bm::word_t*)(block) + bm::set_block_size);
    __m128i m1COshft, m2COshft;
    
    unsigned w0 = *((bm::word_t*)(block));
    unsigned count = 1;

    unsigned co2, co1 = 0;
    for (;block < block_end; block += 2)
    {
        __m128i m1A = _mm_load_si128(block);
        __m128i m2A = _mm_load_si128(block+1);

        __m128i m1CO = _mm_srli_epi32(m1A, 31);
        __m128i m2CO = _mm_srli_epi32(m2A, 31);
        
        co2 = _mm_extract_epi32(m1CO, 3);
        
        __m128i m1As = _mm_slli_epi32(m1A, 1); // (block[i] << 1u)
        __m128i m2As = _mm_slli_epi32(m2A, 1);
        
        m1COshft = _mm_slli_si128 (m1CO, 4); // byte shift left by 1 int32
        m1COshft = _mm_insert_epi32 (m1COshft, co1, 0);
        
        co1 = co2;
        
        co2 = _mm_extract_epi32(m2CO, 3);
        
        m2COshft = _mm_slli_si128 (m2CO, 4);
        m2COshft = _mm_insert_epi32 (m2COshft, co1, 0);
        
        m1As = _mm_or_si128(m1As, m1COshft); // block[i] |= co_flag
        m2As = _mm_or_si128(m2As, m2COshft);
        
        co1 = co2;
        
        // we now have two shifted SSE4 regs with carry-over
        m1A = _mm_xor_si128(m1A, m1As); // w ^= (w >> 1);
        m2A = _mm_xor_si128(m2A, m2As);
        
        bm::id64_t m0 = _mm_extract_epi64(m1A, 0);
        bm::id64_t m1 = _mm_extract_epi64(m1A, 1);
        count += _mm_popcnt_u64(m0) + _mm_popcnt_u64(m1);

        m0 = _mm_extract_epi64(m2A, 0);
        m1 = _mm_extract_epi64(m2A, 1);
        count += _mm_popcnt_u64(m0) + _mm_popcnt_u64(m1);
    }
    count -= (w0 & 1u); // correct initial carry-in error
    return count;
}


#ifdef __GNUG__
// necessary measure to silence false warning from GCC about negative pointer arithmetics
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

/*!
     SSE4.2 check for one to two (variable len) 128 bit SSE lines for gap search results (8 elements)
     @ingroup SSE4
     \internal
*/
inline
unsigned sse4_gap_find(const bm::gap_word_t* BMRESTRICT pbuf, const bm::gap_word_t pos, const unsigned size)
{
    BM_ASSERT(size <= 16);
    BM_ASSERT(size);

    const unsigned unroll_factor = 8;
    if (size < 4) // for very short vector use conventional scan
    {
        unsigned j;
        for (j = 0; j < size; ++j)
        {
            if (pbuf[j] >= pos)
                break;
        }
        return j;
    }

    __m128i m1, mz, maskF, maskFL;

    mz = _mm_setzero_si128();
    m1 = _mm_loadu_si128((__m128i*)(pbuf)); // load first 8 elements

    maskF = _mm_cmpeq_epi64(mz, mz); // set all FF
    maskFL = _mm_slli_si128(maskF, 4 * 2); // byte shift to make [0000 FFFF]
    int shiftL= (64 - (unroll_factor - size) * 16);
    maskFL = _mm_slli_epi64(maskFL, shiftL); // additional bit shift to  [0000 00FF]

    m1 = _mm_andnot_si128(maskFL, m1); // m1 = (~mask) & m1
    m1 = _mm_or_si128(m1, maskFL);

    __m128i mp = _mm_set1_epi16(pos);  // broadcast pos into all elements of a SIMD vector
    __m128i  mge_mask = _mm_cmpeq_epi16(_mm_subs_epu16(mp, m1), mz); // unsigned m1 >= mp
    __m128i  c_mask = _mm_slli_epi16(mge_mask, 15); // clear not needed flag bits by shift
    int mi = _mm_movemask_epi8(c_mask);  // collect flag bits
    if (mi)
    {
        // alternative: int bsr_i= bm::bit_scan_fwd(mi) >> 1;
        unsigned bc = _mm_popcnt_u32(mi); // gives us number of elements >= pos
        return unroll_factor - bc;   // address of first one element (target)
    }
    // inspect the next lane with possible step back (to avoid over-read the block boundaries)
    //   GCC gives a false warning for "- unroll_factor" here
    const bm::gap_word_t* BMRESTRICT pbuf2 = pbuf + size - unroll_factor;
    BM_ASSERT(pbuf2 > pbuf || size == 8); // assert in place to make sure GCC warning is indeed false

    m1 = _mm_loadu_si128((__m128i*)(pbuf2)); // load next elements (with possible overlap)
    mge_mask = _mm_cmpeq_epi16(_mm_subs_epu16(mp, m1), mz); // m1 >= mp        
    mi = _mm_movemask_epi8(_mm_slli_epi16(mge_mask, 15));
    unsigned bc = _mm_popcnt_u32(mi); 

    return size - bc;
}

/**
    Experimental (test) function to do SIMD vector search (lower bound)
    in sorted, growing array
    @ingroup SSE4

    \internal
*/
inline
int sse42_cmpge_u32(__m128i vect4, unsigned value)
{
    // a > b (unsigned, 32-bit) is the same as (a - 0x80000000) > (b - 0x80000000) (signed, 32-bit)
    // https://fgiesen.wordpress.com/2016/04/03/sse-mind-the-gap/
    //
    __m128i mask0x8 = _mm_set1_epi32(0x80000000);
    __m128i mm_val = _mm_set1_epi32(value);
    
    __m128i norm_vect4 = _mm_sub_epi32(vect4, mask0x8); // (signed) vect4 - 0x80000000
    __m128i norm_val = _mm_sub_epi32(mm_val, mask0x8);  // (signed) mm_val - 0x80000000
    
    __m128i cmp_mask_gt = _mm_cmpgt_epi32 (norm_vect4, norm_val);
    __m128i cmp_mask_eq = _mm_cmpeq_epi32 (mm_val, vect4);
    
    __m128i cmp_mask_ge = _mm_or_si128 (cmp_mask_gt, cmp_mask_eq);
    int mask = _mm_movemask_epi8(cmp_mask_ge);
    if (mask)
    {
        int bsf = bm::bsf_asm32(mask);//_bit_scan_forward(mask);   // could use lzcnt()
        return bsf / 4;
    }
    return -1;
}


/**
    lower bound (great or equal) linear scan in ascending order sorted array
    @ingroup SSE4
    \internal
*/
inline
unsigned sse4_lower_bound_scan_u32(const unsigned* BMRESTRICT arr,
                                   unsigned target,
                                   unsigned from,
                                   unsigned to)
{
    // a > b (unsigned, 32-bit) is the same as (a - 0x80000000) > (b - 0x80000000) (signed, 32-bit)
    // see more at:
    // https://fgiesen.wordpress.com/2016/04/03/sse-mind-the-gap/

    const unsigned* BMRESTRICT arr_base = &arr[from]; // unrolled search base
    
    unsigned unroll_factor = 8;
    unsigned len = to - from + 1;
    unsigned len_unr = len - (len % unroll_factor);
    
    __m128i mask0x8 = _mm_set1_epi32(0x80000000);
    __m128i vect_target = _mm_set1_epi32(target);
    __m128i norm_target = _mm_sub_epi32(vect_target, mask0x8);  // (signed) target - 0x80000000

    int mask;
    __m128i vect40, vect41, norm_vect40, norm_vect41, cmp_mask_ge;

    unsigned k = 0;
    for (; k < len_unr; k+=unroll_factor)
    {
        vect40 = _mm_loadu_si128((__m128i*)(&arr_base[k])); // 4 u32s
        norm_vect40 = _mm_sub_epi32(vect40, mask0x8); // (signed) vect4 - 0x80000000
        
        cmp_mask_ge = _mm_or_si128(                              // GT | EQ
                        _mm_cmpgt_epi32 (norm_vect40, norm_target),
                        _mm_cmpeq_epi32 (vect40, vect_target)
                        );
        mask = _mm_movemask_epi8(cmp_mask_ge);
        if (mask)
        {
            int bsf = bm::bsf_asm32(mask); //_bit_scan_forward(mask);
            return from + k + (bsf / 4);
        }
        vect41 = _mm_loadu_si128((__m128i*)(&arr_base[k+4]));
        norm_vect41 = _mm_sub_epi32(vect41, mask0x8);

        cmp_mask_ge = _mm_or_si128(
                        _mm_cmpgt_epi32 (norm_vect41, norm_target),
                        _mm_cmpeq_epi32 (vect41, vect_target)
                        );
        mask = _mm_movemask_epi8(cmp_mask_ge);
        if (mask)
        {
            int bsf = bm::bsf_asm32(mask); //_bit_scan_forward(mask);
            return 4 + from + k + (bsf / 4);
        }
    } // for
    
    for (; k < len; ++k)
    {
        if (arr_base[k] >= target)
            return from + k;
    }
    return to + 1;
}



/*!
     SSE4.2 index lookup to check what belongs to the same block (8 elements)
     \internal
*/
inline
unsigned sse42_idx_arr_block_lookup(const unsigned* idx, unsigned size,
                                   unsigned nb, unsigned start)
{
    const unsigned unroll_factor = 8;
    const unsigned len = (size - start);
    const unsigned len_unr = len - (len % unroll_factor);
    unsigned k;

    idx += start;

    __m128i nbM = _mm_set1_epi32(nb);

    for (k = 0; k < len_unr; k+=unroll_factor)
    {
        __m128i idxA = _mm_loadu_si128((__m128i*)(idx+k));
        __m128i idxB = _mm_loadu_si128((__m128i*)(idx+k+4));
        __m128i nbA =  _mm_srli_epi32(idxA, bm::set_block_shift); // idx[k] >> bm::set_block_shift
        __m128i nbB =  _mm_srli_epi32(idxB, bm::set_block_shift);
        
        if (!_mm_test_all_ones(_mm_cmpeq_epi32(nbM, nbA)) |
            !_mm_test_all_ones(_mm_cmpeq_epi32 (nbM, nbB)))
            break;

    } // for k
    for (; k < len; ++k)
    {
        if (nb != unsigned(idx[k] >> bm::set_block_shift))
            break;
    }
    return start + k;
}

/*!
     SSE4.2 bulk bit set
     \internal
*/
inline
void sse42_set_block_bits(bm::word_t* BMRESTRICT block,
                          const unsigned* BMRESTRICT idx,
                          unsigned start, unsigned stop )
{
    const unsigned unroll_factor = 4;
    const unsigned len = (stop - start);
    const unsigned len_unr = len - (len % unroll_factor);

    idx += start;

    unsigned BM_ALIGN16 mshift_v[4] BM_ALIGN16ATTR;
    unsigned BM_ALIGN16 mword_v[4] BM_ALIGN16ATTR;

    __m128i sb_mask = _mm_set1_epi32(bm::set_block_mask);
    __m128i sw_mask = _mm_set1_epi32(bm::set_word_mask);
    
    unsigned k = 0;
    for (; k < len_unr; k+=unroll_factor)
    {
        __m128i idxA = _mm_loadu_si128((__m128i*)(idx+k));
        __m128i nbitA = _mm_and_si128 (idxA, sb_mask); // nbit = idx[k] & bm::set_block_mask
        __m128i nwordA = _mm_srli_epi32 (nbitA, bm::set_word_shift); // nword  = nbit >> bm::set_word_shift
 
 
        nbitA = _mm_and_si128 (nbitA, sw_mask);
        _mm_store_si128 ((__m128i*)mshift_v, nbitA);

        // check-compare if all 4 bits are in the very same word
        //
        __m128i nwordA_0 = _mm_shuffle_epi32(nwordA, 0x0); // copy element 0
        __m128i cmpA = _mm_cmpeq_epi32(nwordA_0, nwordA);  // compare EQ
        if (_mm_test_all_ones(cmpA)) // check if all are in one word
        {
            unsigned nword = _mm_extract_epi32(nwordA, 0);
            block[nword] |= (1u << mshift_v[0]) | (1u << mshift_v[1])
                            |(1u << mshift_v[2]) | (1u << mshift_v[3]);
        }
        else // bits are in different words, use scalar scatter
        {
            _mm_store_si128 ((__m128i*)mword_v, nwordA);
            
            block[mword_v[0]] |= (1u << mshift_v[0]);
            block[mword_v[1]] |= (1u << mshift_v[1]);
            block[mword_v[2]] |= (1u << mshift_v[2]);
            block[mword_v[3]] |= (1u << mshift_v[3]);
        }

    } // for k

    for (; k < len; ++k)
    {
        unsigned n = idx[k];
        unsigned nbit = unsigned(n & bm::set_block_mask);
        unsigned nword  = nbit >> bm::set_word_shift;
        nbit &= bm::set_word_mask;
        block[nword] |= (1u << nbit);
    } // for k
}


/*!
     SSE4.2 bit block gather-scatter
 
     @param arr - destination array to set bits
     @param blk - source bit-block
     @param idx - gather index array
     @param size - gather array size
     @param start - gaher start index
     @param bit_idx - bit to set in the target array
 
     \internal

    C algorithm:
 
    for (unsigned k = start; k < size; ++k)
    {
        nbit = unsigned(idx[k] & bm::set_block_mask);
        nword  = unsigned(nbit >> bm::set_word_shift);
        mask0 = 1u << (nbit & bm::set_word_mask);
        arr[k] |= TRGW(bool(blk[nword] & mask0) << bit_idx);
    }

*/
inline
void sse4_bit_block_gather_scatter(unsigned* BMRESTRICT arr,
                                   const unsigned* BMRESTRICT blk,
                                   const unsigned* BMRESTRICT idx,
                                   unsigned                   size,
                                   unsigned                   start,
                                   unsigned                   bit_idx)
{
    const unsigned unroll_factor = 4;
    const unsigned len = (size - start);
    const unsigned len_unr = len - (len % unroll_factor);
    
    __m128i sb_mask = _mm_set1_epi32(bm::set_block_mask);
    __m128i sw_mask = _mm_set1_epi32(bm::set_word_mask);
    __m128i maskFF  = _mm_set1_epi32(~0u);
    __m128i maskZ   = _mm_xor_si128(maskFF, maskFF);

    __m128i mask_tmp, mask_0;
    
    unsigned BM_ALIGN16 mshift_v[4] BM_ALIGN16ATTR;
    unsigned BM_ALIGN16 mword_v[4] BM_ALIGN16ATTR;

    unsigned k = 0;
    unsigned base = start + k;
    __m128i* idx_ptr = (__m128i*)(idx + base);   // idx[base]
    __m128i* target_ptr = (__m128i*)(arr + base); // arr[base]
    for (; k < len_unr; k+=unroll_factor)
    {
        __m128i nbitA = _mm_and_si128 (_mm_loadu_si128(idx_ptr), sb_mask); // nbit = idx[base] & bm::set_block_mask
        __m128i nwordA = _mm_srli_epi32 (nbitA, bm::set_word_shift); // nword  = nbit >> bm::set_word_shift
        // (nbit & bm::set_word_mask)
        _mm_store_si128 ((__m128i*)mshift_v, _mm_and_si128 (nbitA, sw_mask));
        _mm_store_si128 ((__m128i*)mword_v, nwordA);
        
        // mask0 = 1u << (nbit & bm::set_word_mask);
        //
#if 0
        // ifdefed an alternative SHIFT implementation using SSE and masks
        // (it is not faster than just doing scalar operations)
        {
        __m128i am_0    = _mm_set_epi32(0, 0, 0, ~0u);
        __m128i mask1   = _mm_srli_epi32 (maskFF, 31);
        mask_0 = _mm_and_si128 (_mm_slli_epi32 (mask1, mshift_v[0]), am_0);
        mask_tmp = _mm_and_si128 (_mm_slli_epi32(mask1, mshift_v[1]), _mm_slli_si128 (am_0, 4));
        mask_0 = _mm_or_si128 (mask_0, mask_tmp);
    
        __m128i mask_2 = _mm_and_si128 (_mm_slli_epi32 (mask1, mshift_v[2]),
                                        _mm_slli_si128 (am_0, 8));
        mask_tmp = _mm_and_si128 (
                      _mm_slli_epi32(mask1, mshift_v[3]),
                      _mm_slli_si128 (am_0, 12)
                      );
    
        mask_0 = _mm_or_si128 (mask_0,
                               _mm_or_si128 (mask_2, mask_tmp)); // assemble bit-test mask
        }
#endif
        mask_0 = _mm_set_epi32(1 << mshift_v[3], 1 << mshift_v[2], 1 << mshift_v[1], 1 << mshift_v[0]);


        //  gather for: blk[nword]  (.. & mask0 )
        //
        mask_tmp = _mm_and_si128(_mm_set_epi32(blk[mword_v[3]], blk[mword_v[2]],
                                               blk[mword_v[1]], blk[mword_v[0]]),
                                 mask_0);
        
        // bool(blk[nword]  ...)
        //maskFF  = _mm_set1_epi32(~0u);
        mask_tmp = _mm_cmpeq_epi32 (mask_tmp, maskZ); // set 0xFF where == 0
        mask_tmp = _mm_xor_si128 (mask_tmp, maskFF); // invert
        mask_tmp = _mm_srli_epi32 (mask_tmp, 31); // (bool) 1 only to the 0 pos
        
        mask_tmp = _mm_slli_epi32(mask_tmp, bit_idx); // << bit_idx
        
        _mm_storeu_si128 (target_ptr,                  // arr[base] |= MASK_EXPR
                          _mm_or_si128 (mask_tmp, _mm_loadu_si128(target_ptr)));
        
        ++idx_ptr; ++target_ptr;
        _mm_prefetch((const char*)target_ptr, _MM_HINT_T0);
    }

    for (; k < len; ++k)
    {
        base = start + k;
        unsigned nbit = unsigned(idx[base] & bm::set_block_mask);
        arr[base] |= unsigned(bool(blk[nbit >> bm::set_word_shift] & (1u << (nbit & bm::set_word_mask))) << bit_idx);
    }

}

/*!
    @brief block shift right by 1
    @ingroup SSE4
*/
inline
bool sse42_shift_r1(__m128i* block, unsigned* empty_acc, unsigned co1)
{
    __m128i* block_end =
        ( __m128i*)((bm::word_t*)(block) + bm::set_block_size);
    __m128i m1COshft, m2COshft;
    __m128i mAcc = _mm_set1_epi32(0);

    unsigned co2;

    for (;block < block_end; block += 2)
    {
        __m128i m1A = _mm_load_si128(block);
        __m128i m2A = _mm_load_si128(block+1);

        __m128i m1CO = _mm_srli_epi32(m1A, 31);
        __m128i m2CO = _mm_srli_epi32(m2A, 31);
        
        co2 = _mm_extract_epi32(m1CO, 3);
        
        m1A = _mm_slli_epi32(m1A, 1); // (block[i] << 1u)
        m2A = _mm_slli_epi32(m2A, 1);
        
        m1COshft = _mm_slli_si128 (m1CO, 4); // byte shift left by 1 int32
        m1COshft = _mm_insert_epi32 (m1COshft, co1, 0);
        
        co1 = co2;
        
        co2 = _mm_extract_epi32(m2CO, 3);
        
        m2COshft = _mm_slli_si128 (m2CO, 4);
        m2COshft = _mm_insert_epi32 (m2COshft, co1, 0);
        
        m1A = _mm_or_si128(m1A, m1COshft); // block[i] |= co_flag
        m2A = _mm_or_si128(m2A, m2COshft);

        _mm_store_si128(block, m1A);
        _mm_store_si128(block+1, m2A);
        
        mAcc = _mm_or_si128(mAcc, m1A);
        mAcc = _mm_or_si128(mAcc, m2A);

        co1 = co2;
    }
    *empty_acc = !_mm_testz_si128(mAcc, mAcc);
    return co1;
}


/*!
    @brief block shift right by 1 plus AND

    @return carry over flag
    @ingroup SSE4
*/
inline
bool sse42_shift_r1_and(__m128i* block,
                        bm::word_t co1,
                        const __m128i* BMRESTRICT mask_block,
                        bm::id64_t* digest)
{
    bm::word_t* wblock = (bm::word_t*) block;
    const bm::word_t* mblock = (const bm::word_t*) mask_block;
    
    __m128i m1COshft, m2COshft;
    __m128i mAcc = _mm_set1_epi32(0);
    unsigned co2;

    bm::id64_t d, wd;
    wd = d = *digest;

    unsigned di = 0;
    if (!co1)
    {
        bm::id64_t t = d & -d;
        di = _mm_popcnt_u64(t - 1); // find start bit-index
    }

    for (; di < 64 ; ++di)
    {
        const unsigned d_base = di * bm::set_block_digest_wave_size;
        bm::id64_t dmask = (1ull << di);
        if (d & dmask) // digest stride NOT empty
        {
            block = (__m128i*) &wblock[d_base];
            mask_block = (__m128i*) &mblock[d_base];
            for (unsigned i = 0; i < 4; ++i, block += 2, mask_block += 2)
            {
                __m128i m1A = _mm_load_si128(block);
                __m128i m2A = _mm_load_si128(block+1);

                __m128i m1CO = _mm_srli_epi32(m1A, 31);
                __m128i m2CO = _mm_srli_epi32(m2A, 31);

                co2 = _mm_extract_epi32(m1CO, 3);
                
                m1A = _mm_slli_epi32(m1A, 1); // (block[i] << 1u)
                m2A = _mm_slli_epi32(m2A, 1);
                
                m1COshft = _mm_slli_si128 (m1CO, 4); // byte shift left by 1 int32
                m1COshft = _mm_insert_epi32 (m1COshft, co1, 0);
                
                co1 = co2;
                
                co2 = _mm_extract_epi32(m2CO, 3);
                
                m2COshft = _mm_slli_si128 (m2CO, 4);
                m2COshft = _mm_insert_epi32 (m2COshft, co1, 0);
                
                m1A = _mm_or_si128(m1A, m1COshft); // block[i] |= co_flag
                m2A = _mm_or_si128(m2A, m2COshft);
                
                m1A = _mm_and_si128(m1A, _mm_load_si128(mask_block)); // block[i] &= mask_block[i]
                m2A = _mm_and_si128(m2A, _mm_load_si128(mask_block+1)); // block[i] &= mask_block[i]

                mAcc = _mm_or_si128(mAcc, m1A);
                mAcc = _mm_or_si128(mAcc, m2A);

                _mm_store_si128(block, m1A);
                _mm_store_si128(block+1, m2A);

                co1 = co2;

            } // for i
            
            if (_mm_testz_si128(mAcc, mAcc))
                d &= ~dmask; // clear digest bit            
            wd &= wd - 1;
        }
        else
        {
            if (co1)
            {
                BM_ASSERT(co1 == 1);
                BM_ASSERT(wblock[d_base] == 0);
                
                unsigned w0 = wblock[d_base] = co1 & mblock[d_base];
                d |= (dmask & (w0 << di)); // update digest (branchless if (w0))
                co1 = 0;
            }
            if (!wd)  // digest is empty, no CO -> exit
                break;
        }
    } // for di
    
    *digest = d;
    return co1;
}


#define VECT_XOR_ARR_2_MASK(dst, src, src_end, mask)\
    sse2_xor_arr_2_mask((__m128i*)(dst), (__m128i*)(src), (__m128i*)(src_end), (bm::word_t)mask)

#define VECT_ANDNOT_ARR_2_MASK(dst, src, src_end, mask)\
    sse2_andnot_arr_2_mask((__m128i*)(dst), (__m128i*)(src), (__m128i*)(src_end), (bm::word_t)mask)

#define VECT_BITCOUNT(first, last) \
    sse4_bit_count((__m128i*) (first), (__m128i*) (last))

#define VECT_BITCOUNT_AND(first, last, mask) \
    sse4_bit_count_op((__m128i*) (first), (__m128i*) (last), (__m128i*) (mask), sse2_and)

#define VECT_BITCOUNT_OR(first, last, mask) \
    sse4_bit_count_op((__m128i*) (first), (__m128i*) (last), (__m128i*) (mask), sse2_or)

#define VECT_BITCOUNT_XOR(first, last, mask) \
    sse4_bit_count_op((__m128i*) (first), (__m128i*) (last), (__m128i*) (mask), sse2_xor)

#define VECT_BITCOUNT_SUB(first, last, mask) \
    sse4_bit_count_op((__m128i*) (first), (__m128i*) (last), (__m128i*) (mask), sse2_sub)

#define VECT_INVERT_BLOCK(first) \
    sse2_invert_block((__m128i*)first);

#define VECT_AND_BLOCK(dst, src) \
    sse4_and_block((__m128i*) dst, (__m128i*) (src))

#define VECT_AND_DIGEST(dst, src) \
    sse4_and_digest((__m128i*) dst, (const __m128i*) (src))

#define VECT_AND_DIGEST_5WAY(dst, src1, src2, src3, src4) \
    sse4_and_digest_5way((__m128i*) dst, (const __m128i*) (src1), (const __m128i*) (src2), (const __m128i*) (src3), (const __m128i*) (src4))

#define VECT_AND_DIGEST_2WAY(dst, src1, src2) \
    sse4_and_digest_2way((__m128i*) dst, (const __m128i*) (src1), (const __m128i*) (src2))

#define VECT_OR_BLOCK(dst, src) \
    sse2_or_block((__m128i*) dst, (__m128i*) (src))

#define VECT_OR_BLOCK_3WAY(dst, src1, src2) \
    sse2_or_block_3way((__m128i*) (dst), (const __m128i*) (src1), (const __m128i*) (src2))

#define VECT_OR_BLOCK_5WAY(dst, src1, src2, src3, src4) \
    sse2_or_block_5way((__m128i*) (dst), (__m128i*) (src1), (__m128i*) (src2), (__m128i*) (src3), (__m128i*) (src4))

#define VECT_SUB_BLOCK(dst, src) \
    sse2_sub_block((__m128i*) dst, (const __m128i*) (src))

#define VECT_SUB_DIGEST(dst, src) \
    sse4_sub_digest((__m128i*) dst, (const __m128i*) (src))

#define VECT_XOR_ARR(dst, src, src_end) \
    sse2_xor_arr((__m128i*) dst, (__m128i*) (src), (__m128i*) (src_end))

#define VECT_COPY_BLOCK(dst, src) \
    sse2_copy_block((__m128i*) dst, (__m128i*) (src))

#define VECT_STREAM_BLOCK(dst, src) \
    sse2_stream_block((__m128i*) dst, (__m128i*) (src))

#define VECT_SET_BLOCK(dst, value) \
    sse2_set_block((__m128i*) dst, value)

#define VECT_IS_ZERO_BLOCK(dst) \
    sse4_is_all_zero((__m128i*) dst)

#define VECT_IS_ONE_BLOCK(dst) \
    sse4_is_all_one((__m128i*) dst)

#define VECT_IS_DIGEST_ZERO(start) \
    sse4_is_digest_zero((__m128i*)start)

#define VECT_LOWER_BOUND_SCAN_U32(arr, target, from, to) \
    sse4_lower_bound_scan_u32(arr, target, from, to)

#define VECT_SHIFT_R1(b, acc, co) \
    sse42_shift_r1((__m128i*)b, acc, co)

#define VECT_SHIFT_R1_AND(b, co, m, digest) \
    sse42_shift_r1_and((__m128i*)b, co, (__m128i*)m, digest)

#define VECT_ARR_BLOCK_LOOKUP(idx, size, nb, start) \
    sse42_idx_arr_block_lookup(idx, size, nb, start)
    
#define VECT_SET_BLOCK_BITS(block, idx, start, stop) \
    sse42_set_block_bits(block, idx, start, stop)

#define VECT_BLOCK_CHANGE(block) \
    sse42_bit_block_calc_change((__m128i*)block)


#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif


#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif


} // namespace




#endif
