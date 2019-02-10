#ifndef BMAGGREGATOR__H__INCLUDED__
#define BMAGGREGATOR__H__INCLUDED__
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

/*! \file bmaggregator.h
    \brief Algorithms for fast aggregation of N bvectors
*/

#include "bm.h"
#include "bmfunc.h"
#include "bmdef.h"

#include "bmalgo_impl.h"


namespace bm
{


/**
    Algorithms for fast aggregation of a group of bit-vectors
 
    Current implementation can aggregate up to max_aggregator_cap vectors
    (TODO: remove this limitation)
 
    Algorithms of this class use cache locality optimizations and efficient
    on cases, wehen we need to apply the same logical operation (aggregate)
    more than 2x vectors.
 
    TARGET = BV1 or BV2 or BV3 or BV4 ...
 
    \ingroup setalgo
*/
template<typename BV>
class aggregator
{
public:
    typedef BV                         bvector_type;
    typedef const bvector_type*        bvector_type_const_ptr;
    typedef bm::id64_t                 digest_type;
    
    typedef typename bvector_type::allocator_type::allocator_pool_type allocator_pool_type;

    /// Maximum aggregation capacity in one pass
    enum max_size
    {
        max_aggregator_cap = 512
    };

    /// Codes for aggregation operations which can be pipelined for efficient execution
    ///
    enum operation
    {
        BM_NOT_DEFINED = 0,
        BM_SHIFT_R_AND = 1
    };

    enum operation_status
    {
        op_undefined = 0,
        op_prepared,
        op_in_progress,
        op_done
    };
    
public:
    aggregator();
    ~aggregator();


    
    // -----------------------------------------------------------------------
    
    /*! @name Methods to setup argument groups and run operations on groups */
    //@{
    
    /**
        Attach source bit-vector to a argument group (0 or 1).
        Arg group 1 used for fused operations like (AND-SUB)
        \param bv - input bit-vector pointer to attach
        \param agr_group - input argument group (0 - default, 1 - fused op)
     
        @return current arg group size (0 if vector was not added (empty))
        @sa reset
    */
    unsigned add(const bvector_type* bv, unsigned agr_group = 0);
    
    /**
        Reset aggregate groups, forget all attached vectors
    */
    void reset();

    /**
        Aggregate added group of vectors using logical OR
        Operation does NOT perform an explicit reset of arg group(s)
     
        \param bv_target - target vector (input is arg group 0)
     
        @sa add, reset
    */
    void combine_or(bvector_type& bv_target);

    /**
        Aggregate added group of vectors using logical AND
        Operation does NOT perform an explicit reset of arg group(s)

        \param bv_target - target vector (input is arg group 0)
     
        @sa add, reset
    */
    void combine_and(bvector_type& bv_target);

    /**
        Aggregate added group of vectors using fused logical AND-SUB
        Operation does NOT perform an explicit reset of arg group(s)

        \param bv_target - target vector (input is arg group 0(AND), 1(SUB) )
     
        \return true if anything was found
     
        @sa add, reset
    */
    bool combine_and_sub(bvector_type& bv_target);
    
    
    /**
        Aggregate added group of vectors using fused logical AND-SUB
        Operation does NOT perform an explicit reset of arg group(s)
        Operation can terminate early if anything was found.

        \param bv_target - target vector (input is arg group 0(AND), 1(SUB) )
        \param any - if true, search result will terminate of first found result

        \return true if anything was found

        @sa add, reset
    */
    bool combine_and_sub(bvector_type& bv_target, bool any);
    
    bool find_first_and_sub(bm::id_t& idx);


    /**
        Aggregate added group of vectors using SHIFT-RIGHT and logical AND
        Operation does NOT perform an explicit reset of arg group(s)

        \param bv_target - target vector (input is arg group 0)
     
        @return bool if anything was found
     
        @sa add, reset
    */
    void combine_shift_right_and(bvector_type& bv_target);
    
    /**
        Set search hint for the range, where results needs to be searched
        (experimental for internal use).
    */
    void set_range_hint(bm::id_t from, bm::id_t to);
    
    //@}
    
    // -----------------------------------------------------------------------
    
    /*! @name Logical operations (C-style interface) */
    //@{

    /**
        Aggregate group of vectors using logical OR
        \param bv_target - target vector
        \param bv_src    - array of pointers on bit-vector aggregate arguments
        \param src_size  - size of bv_src (how many vectors to aggregate)
    */
    void combine_or(bvector_type& bv_target,
                    const bvector_type_const_ptr* bv_src, unsigned src_size);

    /**
        Aggregate group of vectors using logical AND
        \param bv_target - target vector
        \param bv_src    - array of pointers on bit-vector aggregate arguments
        \param src_size  - size of bv_src (how many vectors to aggregate)
    */
    void combine_and(bvector_type& bv_target,
                     const bvector_type_const_ptr* bv_src, unsigned src_size);

    /**
        Fusion aggregate group of vectors using logical AND MINUS another set
     
        \param bv_target     - target vector
        \param bv_src_and    - array of pointers on bit-vectors for AND
        \param src_and_size  - size of AND group
        \param bv_src_sub    - array of pointers on bit-vectors for SUBstract
        \param src_sub_size  - size of SUB group
        \param any           - flag if caller needs any results asap (incomplete results)
     
        \return true when found
    */
    bool combine_and_sub(bvector_type& bv_target,
                     const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
                     const bvector_type_const_ptr* bv_src_sub, unsigned src_sub_size,
                     bool any);
    
    bool find_first_and_sub(bm::id_t& idx,
                     const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
                     const bvector_type_const_ptr* bv_src_sub, unsigned src_sub_size);

    /**
        Fusion aggregate group of vectors using SHIFT right with AND
     
        \param bv_target     - target vector
        \param bv_src_and    - array of pointers on bit-vectors for AND masking
        \param src_and_size  - size of AND group
        \param any           - flag if caller needs any results asap (incomplete results)
     
        \return true when found
    */
    bool combine_shift_right_and(bvector_type& bv_target,
            const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
            bool any);

    
    //@}

    // -----------------------------------------------------------------------

    /*! @name Horizontal Logical operations used for tests (C-style interface) */
    //@{
    
    /**
        Horizontal OR aggregation (potentially slower) method.
        \param bv_target - target vector
        \param bv_src    - array of pointers on bit-vector aggregate arguments
        \param src_size  - size of bv_src (how many vectors to aggregate)
    */
    void combine_or_horizontal(bvector_type& bv_target,
                               const bvector_type_const_ptr* bv_src, unsigned src_size);
    /**
        Horizontal AND aggregation (potentially slower) method.
        \param bv_target - target vector
        \param bv_src    - array of pointers on bit-vector aggregate arguments
        \param src_size  - size of bv_src (how many vectors to aggregate)
    */
    void combine_and_horizontal(bvector_type& bv_target,
                                const bvector_type_const_ptr* bv_src, unsigned src_size);

    /**
        Horizontal AND-SUB aggregation (potentially slower) method.
        \param bv_target - target vector
        \param bv_src_and    - array of pointers on bit-vector to AND aggregate
        \param src_and_size  - size of bv_src_and
        \param bv_src_sub    - array of pointers on bit-vector to SUB aggregate
        \param src_sub_size  - size of bv_src_sub
    */
    void combine_and_sub_horizontal(bvector_type& bv_target,
                                    const bvector_type_const_ptr* bv_src_and,
                                    unsigned src_and_size,
                                    const bvector_type_const_ptr* bv_src_sub,
                                    unsigned src_sub_size);

    //@}
    

    // -----------------------------------------------------------------------

    /*! @name Pipeline opreations */
    //@{

    /** Get current operation code */
    int get_operation() const { return operation_; }

    /** Set operation code for the aggregator */
    void set_operation(int op_code) { operation_ = op_code; }

    /**
        Prepare operation, create internal resources, analyse dependencies.
        Prerequisites are: that operation is set and all argument vectors are added
    */
    void stage(bm::word_t* temp_block);
    
    /**
        Run a step of current arrgegation operation
    */
    operation_status run_step(unsigned i, unsigned j);
    
    operation_status get_operation_status() const { return operation_status_; }
    
    const bvector_type* get_target() const { return bv_target_; }
    
    bm::word_t* get_temp_block() { return ar_->tb1; }
    
    //@}

protected:
    typedef typename bvector_type::blocks_manager_type blocks_manager_type;
    

    void combine_or(unsigned i, unsigned j,
                    bvector_type& bv_target,
                    const bvector_type_const_ptr* bv_src, unsigned src_size);

    void combine_and(unsigned i, unsigned j,
                    bvector_type& bv_target,
                    const bvector_type_const_ptr* bv_src, unsigned src_size);
    
    digest_type combine_and_sub(unsigned i, unsigned j,
                         const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
                         const bvector_type_const_ptr* bv_src_sub, unsigned src_sub_size);
    
    void prepare_shift_right_and(bvector_type& bv_target,
                                 const bvector_type_const_ptr* bv_src, unsigned src_size);

    bool combine_shift_right_and(unsigned i, unsigned j,
                                 bvector_type& bv_target,
                                 const bvector_type_const_ptr* bv_src, unsigned src_size);


    static
    unsigned resize_target(bvector_type& bv_target,
                           const bvector_type_const_ptr* bv_src,
                           unsigned src_size,
                           bool init_clear = true);
    
    bm::word_t* sort_input_blocks_or(const bvector_type_const_ptr* bv_src,
                                     unsigned src_size,
                                     unsigned i, unsigned j,
                                     unsigned* arg_blk_count,
                                     unsigned* arg_blk_gap_count);
    
    bm::word_t* sort_input_blocks_and(const bvector_type_const_ptr* bv_src,
                                      unsigned src_size,
                                      unsigned i, unsigned j,
                                      unsigned* arg_blk_count,
                                      unsigned* arg_blk_gap_count);


    bool process_bit_blocks_or(blocks_manager_type& bman_target,
                               unsigned i, unsigned j, unsigned block_count);

    bool process_gap_blocks_or(blocks_manager_type& bman_target,
                               unsigned i, unsigned j, unsigned block_count);
    
    digest_type process_bit_blocks_and(unsigned block_count, digest_type digest);
    
    digest_type process_gap_blocks_and(unsigned block_count, digest_type digest);

    bool test_gap_blocks_and(unsigned block_count, unsigned bit_idx);
    bool test_gap_blocks_sub(unsigned block_count, unsigned bit_idx);

    digest_type process_bit_blocks_sub(unsigned block_count, digest_type digest);

    digest_type process_gap_blocks_sub(unsigned block_count, digest_type digest);

    static
    unsigned find_effective_sub_block_size(unsigned i,
                                           const bvector_type_const_ptr* bv_src,
                                           unsigned src_size);
    
    bool any_carry_overs(unsigned co_size) const;
    
    /**
        @return carry over
    */
    bool process_shift_right_and(const bm::word_t* arg_blk,
                                     digest_type&      digest,
                                     unsigned          carry_over);
    
    const bm::word_t* get_arg_block(const bvector_type_const_ptr* bv_src,
                                    unsigned k, unsigned i, unsigned j);

    bvector_type* check_create_target();
    
private:
    /// Memory arena for logical operations
    ///
    /// @internal
    struct arena
    {
        BM_DECLARE_TEMP_BLOCK(tb1);
        // commented out blocks are used in experimental GAP aggregations
#if 0
        bm::gap_word_t        gap_res_buf1[bm::gap_equiv_len * 3]; ///< temp 1
        bm::gap_word_t        gap_res_buf2[bm::gap_equiv_len * 3]; ///< temp 2
        bm::gap_word_t        gap_res_buf3[bm::gap_equiv_len * 6]; ///< temp 3
#endif
        const bm::word_t*     v_arg_blk[max_aggregator_cap];     ///< source blocks list
        const bm::gap_word_t* v_arg_blk_gap[max_aggregator_cap]; ///< source GAP blocks list
        
        bvector_type_const_ptr arg_bv0[max_aggregator_cap]; ///< arg group 0
        bvector_type_const_ptr arg_bv1[max_aggregator_cap]; ///< arg group 1
        unsigned char          carry_overs_[max_aggregator_cap]; /// carry over flags
    };
    
    aggregator(const aggregator&) = delete;
    aggregator& operator=(const aggregator&) = delete;
    
private:
    arena*               ar_; ///< data arena ptr (heap allocated)
    unsigned             arg_group0_size = 0;
    unsigned             arg_group1_size = 0;
    allocator_pool_type  pool_; ///< pool for operations with cyclic mem.use

    bm::word_t*          temp_blk_= 0;   ///< external temp block ptr
    int                  operation_ = 0; ///< operation code (default: not defined)
    operation_status     operation_status_ = op_undefined;
    bvector_type*        bv_target_ = 0; ///< target bit-vector
    unsigned             top_block_size_ = 0; ///< operation top block (i) size
    
    // search range setting (hint) [from, to]
    bool                 range_set_ = false; ///< range flag
    bm::id_t             range_from_ = bm::id_max; ///< search from
    bm::id_t             range_to_   = bm::id_max; ///< search to

};




// ------------------------------------------------------------------------
//
// ------------------------------------------------------------------------


template<typename Agg, typename It>
void aggregator_pipeline_execute(It  first, It last)
{
    bm::word_t* tb = (*first)->get_temp_block();

    int pipeline_size = 0;
    for (It it = first; it != last; ++it, ++pipeline_size)
    {
        Agg& agg = *(*it);
        agg.stage(tb);
    }
    for (unsigned i = 0; i < bm::set_array_size; ++i)
    {
        unsigned j = 0;
        do
        {
            // run all aggregators for the [i,j] coordinate
            unsigned w = 0;
            for (It it = first; it != last; ++it, ++w)
            {
                Agg& agg = *(*it);
                auto op_st = agg.get_operation_status();
                if (op_st != Agg::op_done)
                {
                    op_st = agg.run_step(i, j);
                    pipeline_size -= (op_st == Agg::op_done);
                }
            } // for it
            if (pipeline_size <= 0)
            {
                return;
            }

        } while (++j < bm::set_array_size);

    } // for i
}


// ------------------------------------------------------------------------
//
// ------------------------------------------------------------------------


template<typename BV>
aggregator<BV>::aggregator()
{
    ar_ = (arena*) bm::aligned_new_malloc(sizeof(arena));
}

// ------------------------------------------------------------------------

template<typename BV>
aggregator<BV>::~aggregator()
{
    BM_ASSERT(ar_);
    bm::aligned_free(ar_);
    delete bv_target_; 
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::reset()
{
    arg_group0_size = arg_group1_size = operation_ = top_block_size_ = 0;
    operation_status_ = op_undefined;
    range_set_ = false;
    range_from_ = range_to_ = bm::id_max;
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::set_range_hint(bm::id_t from, bm::id_t to)
{
    range_from_ = from; range_to_ = to;
    range_set_ = true;
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::bvector_type* aggregator<BV>::check_create_target()
{
    if (!bv_target_)
    {
        bv_target_ = new bvector_type();
        bv_target_->init();
    }
    return bv_target_;
}

// ------------------------------------------------------------------------

template<typename BV>
unsigned aggregator<BV>::add(const bvector_type* bv, unsigned agr_group)
{
    BM_ASSERT_THROW(agr_group <= 1, BM_ERR_RANGE);
    BM_ASSERT(agr_group <= 1);
    
    if (agr_group) // arg group 1
    {
        BM_ASSERT(arg_group1_size < max_aggregator_cap);
        BM_ASSERT_THROW(arg_group1_size < max_aggregator_cap, BM_ERR_RANGE);
        
        if (!bv)
            return arg_group1_size;
        
        ar_->arg_bv1[arg_group1_size++] = bv;
        return arg_group1_size;
    }
    else // arg group 0
    {
        BM_ASSERT(arg_group0_size < max_aggregator_cap);
        BM_ASSERT_THROW(arg_group0_size < max_aggregator_cap, BM_ERR_RANGE);

        if (!bv)
            return arg_group0_size;

        ar_->arg_bv0[arg_group0_size++] = bv;
        return arg_group0_size;
    }
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_or(bvector_type& bv_target)
{
    combine_or(bv_target, ar_->arg_bv0, arg_group0_size);
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_and(bvector_type& bv_target)
{
    combine_and(bv_target, ar_->arg_bv0, arg_group0_size);
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::combine_and_sub(bvector_type& bv_target)
{
    return combine_and_sub(bv_target,
                    ar_->arg_bv0, arg_group0_size,
                    ar_->arg_bv1, arg_group1_size, false);
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::combine_and_sub(bvector_type& bv_target, bool any)
{
    return combine_and_sub(bv_target,
                    ar_->arg_bv0, arg_group0_size,
                    ar_->arg_bv1, arg_group1_size, any);
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::find_first_and_sub(bm::id_t& idx)
{
    return find_first_and_sub(idx,
                        ar_->arg_bv0, arg_group0_size,
                        ar_->arg_bv1, arg_group1_size);
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_shift_right_and(bvector_type& bv_target)
{
    combine_shift_right_and(bv_target, ar_->arg_bv0, arg_group0_size, false);
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_or(bvector_type& bv_target,
                        const bvector_type_const_ptr* bv_src, unsigned src_size)
{
    BM_ASSERT_THROW(src_size < max_aggregator_cap, BM_ERR_RANGE);
    if (!src_size)
    {
        bv_target.clear();
        return;
    }

    unsigned top_blocks = resize_target(bv_target, bv_src, src_size);
    for (unsigned i = 0; i < top_blocks; ++i)
    {
        unsigned set_array_max = find_effective_sub_block_size(i, bv_src, src_size);
        unsigned j = 0;
        do
        {
            combine_or(i, j, bv_target, bv_src, src_size);
        } while (++j < set_array_max);
    } // for i
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_and(bvector_type& bv_target,
                        const bvector_type_const_ptr* bv_src, unsigned src_size)
{
    BM_ASSERT_THROW(src_size < max_aggregator_cap, BM_ERR_RANGE);

    if (!src_size)
    {
        bv_target.clear();
        return;
    }

    unsigned top_blocks = resize_target(bv_target, bv_src, src_size);
    
    for (unsigned i = 0; i < top_blocks; ++i)
    {
        unsigned set_array_max = find_effective_sub_block_size(i, bv_src, src_size);
        unsigned j = 0;
        do
        {
            combine_and(i, j, bv_target, bv_src, src_size);
        } while (++j < set_array_max);
    } // for i
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::combine_and_sub(bvector_type& bv_target,
                 const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
                 const bvector_type_const_ptr* bv_src_sub, unsigned src_sub_size,
                 bool any)
{
    BM_ASSERT_THROW(src_and_size < max_aggregator_cap, BM_ERR_RANGE);
    BM_ASSERT_THROW(src_sub_size < max_aggregator_cap, BM_ERR_RANGE);
    
    bool global_found = false;

    if (!bv_src_and || !src_and_size)
    {
        bv_target.clear();
        return false;
    }

    unsigned top_blocks = resize_target(bv_target, bv_src_and, src_and_size);
    unsigned top_blocks2 = resize_target(bv_target, bv_src_sub, src_sub_size, false);
    
    if (top_blocks2 > top_blocks)
        top_blocks = top_blocks2;

    for (unsigned i = 0; i < top_blocks; ++i)
    {
        unsigned set_array_max = find_effective_sub_block_size(i, bv_src_and, src_and_size);
        if (src_sub_size)
        {
            unsigned set_array_max2 =
                    find_effective_sub_block_size(i, bv_src_sub, src_sub_size);
            if (set_array_max2 > set_array_max)
                set_array_max = set_array_max2;
        }
        unsigned j = 0;
        do
        {
            digest_type digest = combine_and_sub(i, j,
                                                bv_src_and, src_and_size,
                                                bv_src_sub, src_sub_size);
            bool found = digest;
            if (found)
            {
                blocks_manager_type& bman_target = bv_target.get_blocks_manager();
                bman_target.copy_bit_block(i, j, ar_->tb1);
                if (any)
                    return found;
            }
            global_found |= found;
        } while (++j < set_array_max);
    } // for i
    return global_found;
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::find_first_and_sub(bm::id_t& idx,
                 const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
                 const bvector_type_const_ptr* bv_src_sub, unsigned src_sub_size)
{
    BM_ASSERT_THROW(src_and_size < max_aggregator_cap, BM_ERR_RANGE);
    BM_ASSERT_THROW(src_sub_size < max_aggregator_cap, BM_ERR_RANGE);
    
    if (!bv_src_and || !src_and_size)
        return false;

    BV bv_target; // TODO: get rid of it
    unsigned top_blocks = resize_target(bv_target, bv_src_and, src_and_size);
    unsigned top_blocks2 = resize_target(bv_target, bv_src_sub, src_sub_size, false);
    
    if (top_blocks2 > top_blocks)
        top_blocks = top_blocks2;
    
    // compute range blocks coordinates
    //
    unsigned nblock_from = unsigned(range_from_ >> bm::set_block_shift);
    unsigned nblock_to = unsigned(range_to_ >> bm::set_block_shift);
    unsigned top_from = nblock_from >> bm::set_array_shift;
    unsigned top_to = nblock_to >> bm::set_array_shift;
    
    if (range_set_)
    {
        if (nblock_from == nblock_to) // one block search
        {
            unsigned i = top_from;
            unsigned j = nblock_from & bm::set_array_mask;
            digest_type digest = combine_and_sub(i, j,
                                                 bv_src_and, src_and_size,
                                                 bv_src_sub, src_sub_size);
            if (digest)
            {
                // TODO: optimize search using digest
                unsigned block_bit_idx = 0;
                bool found = bm::bit_find_first(ar_->tb1, &block_bit_idx, digest);
                BM_ASSERT(found);
                idx = bm::block_to_global_index(i, j, block_bit_idx);
                return found;
            }
            return false;
        }

        if (top_to < top_blocks)
            top_blocks = top_to+1;
    }
    else
    {
        top_from = 0;
    }
    
    for (unsigned i = top_from; i < top_blocks; ++i)
    {
        unsigned set_array_max = bm::set_array_size;
        unsigned j = 0;
        
        if (range_set_)
        {
            if (i == top_from)
            {
                j = nblock_from & bm::set_array_mask;
            }
            if (i == top_to)
            {
                set_array_max = 1 + (nblock_to & bm::set_array_mask);
            }
        }
        else
        {
            set_array_max = find_effective_sub_block_size(i, bv_src_and, src_and_size);
            if (src_sub_size)
            {
                unsigned set_array_max2 =
                        find_effective_sub_block_size(i, bv_src_sub, src_sub_size);
                if (set_array_max2 > set_array_max)
                    set_array_max = set_array_max2;
            }
        }
        do
        {
            digest_type digest = combine_and_sub(i, j,
                                                bv_src_and, src_and_size,
                                                bv_src_sub, src_sub_size);
            if (digest)
            {
                // TODO: optimize search using digest
                unsigned block_bit_idx = 0;
                bool found = bm::bit_find_first(ar_->tb1, &block_bit_idx, digest);
                BM_ASSERT(found);
                idx = bm::block_to_global_index(i, j, block_bit_idx);
                return found;
            }
        } while (++j < set_array_max);
    } // for i
    return false;
}

// ------------------------------------------------------------------------

template<typename BV>
unsigned
aggregator<BV>::find_effective_sub_block_size(unsigned i,
                                              const bvector_type_const_ptr* bv_src,
                                              unsigned src_size) 
{
    // quick hack to avoid scanning large, arrays, where such scan can be quite
    // expensive by itself (this makes this function approximate)
    if (src_size > 32)
        return bm::set_array_size;

    unsigned max_size = 1;
    for (unsigned k = 0; k < src_size; ++k)
    {
        const bvector_type* bv = bv_src[k];
        BM_ASSERT(bv);
        const typename bvector_type::blocks_manager_type& bman_arg =
                                                    bv->get_blocks_manager();
        const bm::word_t* const* blk_blk_arg = bman_arg.get_topblock(i);
        if (!blk_blk_arg)
            continue;

        for (unsigned j = bm::set_array_size - 1; j > max_size; --j)
        {
            if (blk_blk_arg[j])
            {
                max_size = j;
                break;
            }
        }
    } // for k
    ++max_size;
    BM_ASSERT(max_size <= bm::set_array_size);
    
    return max_size;
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_or(unsigned i, unsigned j,
                                bvector_type& bv_target,
                                const bvector_type_const_ptr* bv_src,
                                unsigned src_size)
{
    typename bvector_type::blocks_manager_type& bman_target = bv_target.get_blocks_manager();

    unsigned arg_blk_count = 0;
    unsigned arg_blk_gap_count = 0;
    bm::word_t* blk =
        sort_input_blocks_or(bv_src, src_size, i, j,
                             &arg_blk_count, &arg_blk_gap_count);

    BM_ASSERT(blk == 0 || blk == FULL_BLOCK_FAKE_ADDR);

    if (blk == FULL_BLOCK_FAKE_ADDR) // nothing to do - golden block(!)
    {
        bman_target.check_alloc_top_subblock(i);
        bman_target.set_block_ptr(i, j, blk);
    }
    else
    {
        blk = ar_->tb1;
        if (arg_blk_count || arg_blk_gap_count)
        {
            bool all_one =
                process_bit_blocks_or(bman_target, i, j, arg_blk_count);
            if (!all_one)
            {
                if (arg_blk_gap_count)
                {
                    all_one =
                        process_gap_blocks_or(bman_target, i, j, arg_blk_gap_count);
                }
                if (!all_one)
                {
                    // we have some results, allocate block and copy from temp
                    bman_target.copy_bit_block(i, j, ar_->tb1);
                }
            }
        }
    }

}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_and(unsigned i, unsigned j,
                                 bvector_type& bv_target,
                                 const bvector_type_const_ptr* bv_src,
                                 unsigned src_size)
{
    BM_ASSERT(src_size);
    
    typename bvector_type::blocks_manager_type& bman_target = bv_target.get_blocks_manager();

    unsigned arg_blk_count = 0;
    unsigned arg_blk_gap_count = 0;
    bm::word_t* blk =
        sort_input_blocks_and(bv_src, src_size,
                              i, j,
                              &arg_blk_count, &arg_blk_gap_count);

    BM_ASSERT(blk == 0 || blk == FULL_BLOCK_FAKE_ADDR);

    if (!blk) // nothing to do - golden block(!)
        return;
    if (arg_blk_count || arg_blk_gap_count)
    {
        // AND bit-blocks
        //
        bm::id64_t digest = 0;
        digest = process_bit_blocks_and(arg_blk_count, digest);
        if (!digest)
            return;

        // AND all GAP blocks (if any)
        //
        if (arg_blk_gap_count)
        {
            digest =
                process_gap_blocks_and(arg_blk_gap_count, digest);
        }
        if (digest) // some results
        {
            // we have some results, allocate block and copy from temp
            bman_target.copy_bit_block(i, j, ar_->tb1);
        }
    }
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::digest_type
aggregator<BV>::combine_and_sub(unsigned i, unsigned j,
             const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
             const bvector_type_const_ptr* bv_src_sub, unsigned src_sub_size)
{
    BM_ASSERT(src_and_size);
    unsigned arg_blk_and_count = 0;
    unsigned arg_blk_and_gap_count = 0;
    unsigned arg_blk_sub_count = 0;
    unsigned arg_blk_sub_gap_count = 0;

    bm::word_t* blk = sort_input_blocks_and(bv_src_and, src_and_size,
                                              i, j,
                                   &arg_blk_and_count, &arg_blk_and_gap_count);

    BM_ASSERT(blk == 0 || blk == FULL_BLOCK_FAKE_ADDR);

    if (!blk || !(arg_blk_and_count | arg_blk_and_gap_count))
        return 0; // nothing to do - golden block(!)

    unsigned single_bit_idx = 0;
    bool single_bit_found = false;
    digest_type digest = 0;
    
    // AND bit-blocks
    //
    digest = process_bit_blocks_and(arg_blk_and_count, digest);
    if (!digest)
        return digest;
    
    single_bit_found = bm::bit_find_first_if_1(ar_->tb1, &single_bit_idx, digest);
    
    // AND all GAP blocks (if any)
    //
    if (arg_blk_and_gap_count)
    {
        if (single_bit_found) // use logical check in GAP blocks
        {
            bool found =
                test_gap_blocks_and(arg_blk_and_gap_count, single_bit_idx);
            if (!found)
                return 0;
        }
        else
        {
            digest =
                process_gap_blocks_and(arg_blk_and_gap_count, digest);
        }
        if (!digest)
            return digest;
    }
    
    if (src_sub_size)
    {
        blk = sort_input_blocks_or(bv_src_sub, src_sub_size,
                                   i, j,
                                   &arg_blk_sub_count, &arg_blk_sub_gap_count);
        
        BM_ASSERT(blk == 0 || blk == FULL_BLOCK_FAKE_ADDR);
        if (blk == FULL_BLOCK_FAKE_ADDR)
            return 0; // nothing to do - golden block(!)
        
        if (arg_blk_sub_count || arg_blk_sub_gap_count)
        {
            digest = process_bit_blocks_sub(arg_blk_sub_count, digest);
            if (!digest)
                return digest;

            if (arg_blk_sub_gap_count)
            {
                bool found = bm::bit_find_first_if_1(ar_->tb1, &single_bit_idx,
                                                     digest);
                if (found)
                {
                    // AND-NOT for 1 single bit
                    found = test_gap_blocks_sub(arg_blk_sub_gap_count, single_bit_idx);
                    if (!found)
                        digest = 0;
                    return digest;
                }
                digest =
                    process_gap_blocks_sub(arg_blk_sub_gap_count, digest);
            }
        }
    }

    return digest;
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::process_gap_blocks_or(blocks_manager_type& bman_target,
                                           unsigned i, unsigned j,
                                           unsigned arg_blk_gap_count)
{
    bm::word_t* blk = ar_->tb1;
    bool all_one;

    unsigned k = 0;
// experimental code, commented out as inefficient
#if 0
    unsigned unroll_factor = 4;
    unsigned len = arg_blk_gap_count - k;
    unsigned len_unr = len - (len % unroll_factor);
    
    const bm::gap_word_t* res;
    unsigned res_len;

    for (; k < len_unr; k+=unroll_factor)
    {
        {
            const bm::gap_word_t* gap1 = db_->v_arg_blk_gap[k];
            const bm::gap_word_t* gap2 = db_->v_arg_blk_gap[k+1];
            res = bm::gap_operation_or(gap1, gap2, db_->gap_res_buf1, res_len);
            BM_ASSERT(res == db_->gap_res_buf1);
            if (bm::gap_is_all_one(res, bm::gap_max_bits))
            {
                bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
                return true; // golden block found (all 1s)!
            }
        }
        {
            const bm::gap_word_t* gap1 = db_->v_arg_blk_gap[k+2];
            const bm::gap_word_t* gap2 = db_->v_arg_blk_gap[k+3];
            res = bm::gap_operation_or(gap1, gap2, db_->gap_res_buf2, res_len);
            BM_ASSERT(res == db_->gap_res_buf2);
            if (bm::gap_is_all_one(res, bm::gap_max_bits))
            {
                bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
                return true; // golden block found (all 1s)!
            }
        }
        
        res = bm::gap_operation_or(db_->gap_res_buf1, db_->gap_res_buf2, db_->gap_res_buf3, res_len);
        BM_ASSERT(res == db_->gap_res_buf3);
        if (bm::gap_is_all_one(res, bm::gap_max_bits))
        {
            bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
            return true; // golden block found (all 1s)!
        }
        
        // accumulate the result of 4x gap OR
        bm::gap_add_to_bitset(blk, res);
    }
    unroll_factor = 2;
    len = arg_blk_gap_count - k;
    len_unr = len - (len % unroll_factor);

    for (; k < len_unr; k+=unroll_factor)
    {
        {
            const bm::gap_word_t* gap1 = db_->v_arg_blk_gap[k];
            const bm::gap_word_t* gap2 = db_->v_arg_blk_gap[k+1];
            res = bm::gap_operation_or(gap1, gap2, db_->gap_res_buf1, res_len);
            BM_ASSERT(res == db_->gap_res_buf1);
            if (bm::gap_is_all_one(res, bm::gap_max_bits))
            {
                bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
                return true; // golden block found (all 1s)!
            }
        }
        // accumulate the result of 2x gap OR
        bm::gap_add_to_bitset(blk, res);
    }
#endif

    for (; k < arg_blk_gap_count; ++k)
    {
        bm::gap_add_to_bitset(blk, ar_->v_arg_blk_gap[k]);
    }
    
    all_one = bm::is_bits_one((bm::wordop_t*) blk);
    if (all_one)
    {
        bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
        return true;
    }
    
    return false;
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::digest_type
aggregator<BV>::process_gap_blocks_and(unsigned    arg_blk_gap_count,
                                       digest_type digest)
{
    BM_ASSERT(arg_blk_gap_count);
    BM_ASSERT(digest);

    bm::word_t* blk = ar_->tb1;
    bool single_bit_found;
    unsigned single_bit_idx;
    for (unsigned k = 0; k < arg_blk_gap_count; ++k)
    {
        bm::gap_and_to_bitset(blk, ar_->v_arg_blk_gap[k], digest);
        digest = bm::update_block_digest0(blk, digest);
        if (!digest)
        {
            BM_ASSERT(bm::bit_is_all_zero(blk));
            break;
        }
        single_bit_found = bm::bit_find_first_if_1(blk, &single_bit_idx, digest);
        if (single_bit_found)
        {
            for (++k; k < arg_blk_gap_count; ++k)
            {
                bool b = bm::gap_test_unr(ar_->v_arg_blk_gap[k], single_bit_idx);
                if (!b)
                    return 0; // AND 0 causes result to turn 0
            }
            break;
        }
    }
    return digest;
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::digest_type
aggregator<BV>::process_gap_blocks_sub(unsigned   arg_blk_gap_count,
                                                   digest_type digest)
{
    BM_ASSERT(arg_blk_gap_count);
    BM_ASSERT(digest);

    bm::word_t* blk = ar_->tb1;
    bool single_bit_found;
    unsigned single_bit_idx;
    for (unsigned k = 0; k < arg_blk_gap_count; ++k)
    {
        bm::gap_sub_to_bitset(blk, ar_->v_arg_blk_gap[k], digest);
        digest = bm::update_block_digest0(blk, digest);
        if (!digest)
        {
            BM_ASSERT(bm::bit_is_all_zero(blk));
            break;
        }
        // check if logical operation reduced to a corner case of one single bit
        single_bit_found = bm::bit_find_first_if_1(blk, &single_bit_idx, digest);
        if (single_bit_found)
        {
            for (++k; k < arg_blk_gap_count; ++k)
            {
                bool b = bm::gap_test_unr(ar_->v_arg_blk_gap[k], single_bit_idx);
                if (b)
                    return 0; // AND-NOT causes search result to turn 0
            } // for k
            break;
        }
    } // for k
    return digest;
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::test_gap_blocks_and(unsigned arg_blk_gap_count,
                                         unsigned bit_idx)
{
    unsigned b = 1;
    for (unsigned i = 0; i < arg_blk_gap_count && b; ++i)
    {
        b = bm::gap_test_unr(ar_->v_arg_blk_gap[i], bit_idx);
    }
    return b;
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::test_gap_blocks_sub(unsigned arg_blk_gap_count,
                                         unsigned bit_idx)
{
    unsigned b = 1;
    for (unsigned i = 0; i < arg_blk_gap_count; ++i)
    {
        b = bm::gap_test_unr(ar_->v_arg_blk_gap[i], bit_idx);
        if (b)
            return false;
    }
    return true;
}

// ------------------------------------------------------------------------


template<typename BV>
bool aggregator<BV>::process_bit_blocks_or(blocks_manager_type& bman_target,
                                           unsigned i, unsigned j,
                                           unsigned arg_blk_count)
{
    bm::word_t* blk = ar_->tb1;
    bool all_one;

    unsigned k = 0;

    if (arg_blk_count)  // copy the first block
        bm::bit_block_copy(blk, ar_->v_arg_blk[k++]);
    else
        bm::bit_block_set(blk, 0);

    // process all BIT blocks
    //
    unsigned unroll_factor, len, len_unr;
    
    unroll_factor = 4;
    len = arg_blk_count - k;
    len_unr = len - (len % unroll_factor);
    for( ;k < len_unr; k+=unroll_factor)
    {
        all_one = bm::bit_block_or_5way(blk,
                                        ar_->v_arg_blk[k], ar_->v_arg_blk[k+1],
                                        ar_->v_arg_blk[k+2], ar_->v_arg_blk[k+3]);
        if (all_one)
        {
            BM_ASSERT(blk == ar_->tb1);
            BM_ASSERT(bm::is_bits_one((bm::wordop_t*) blk));
            bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
            return true;
        }
    } // for k

    unroll_factor = 2;
    len = arg_blk_count - k;
    len_unr = len - (len % unroll_factor);
    for( ;k < len_unr; k+=unroll_factor)
    {
        all_one = bm::bit_block_or_3way(blk, ar_->v_arg_blk[k], ar_->v_arg_blk[k+1]);
        if (all_one)
        {
            BM_ASSERT(blk == ar_->tb1);
            BM_ASSERT(bm::is_bits_one((bm::wordop_t*) blk));
            bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
            return true;
        }
    } // for k

    for (; k < arg_blk_count; ++k)
    {
        all_one = bm::bit_block_or(blk, ar_->v_arg_blk[k]);
        if (all_one)
        {
            BM_ASSERT(blk == ar_->tb1);
            BM_ASSERT(bm::is_bits_one((bm::wordop_t*) blk));
            bman_target.set_block(i, j, FULL_BLOCK_FAKE_ADDR, false);
            return true;
        }
    } // for k

    return false;
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::digest_type
aggregator<BV>::process_bit_blocks_and(unsigned   arg_blk_count,
                                       digest_type digest)
{
    bm::word_t* blk = ar_->tb1;
    unsigned k = 0;
    switch (arg_blk_count)
    {
    case 0:
        bm::bit_block_set(ar_->tb1, ~0u); // set buffer to 0xFF...
        return ~0ull;
    case 1:
        bm::bit_block_copy(blk, ar_->v_arg_blk[k]);
        return bm::calc_block_digest0(blk);
    default:
        digest = bm::bit_block_and_2way(blk,
                                        ar_->v_arg_blk[k],
                                        ar_->v_arg_blk[k+1],
                                        ~0ull);
        k += 2;
        break;
    } // switch

    unsigned unroll_factor, len, len_unr;
    unsigned single_bit_idx;

    unroll_factor = 4;
    len = arg_blk_count - k;
    len_unr = len - (len % unroll_factor);
    for (; k < len_unr; k += unroll_factor)
    {
        digest = 
            bm::bit_block_and_5way(blk, 
                                   ar_->v_arg_blk[k], ar_->v_arg_blk[k + 1],
                                   ar_->v_arg_blk[k + 2], ar_->v_arg_blk[k + 3],
                                   digest);
        if (!digest) // all zero
            return digest;
        bool found = bm::bit_find_first_if_1(blk, &single_bit_idx, digest);
        if (found)
        {
            unsigned nword = unsigned(single_bit_idx >> bm::set_word_shift);
            unsigned mask = 1u << (single_bit_idx & bm::set_word_mask);
            for (++k; k < arg_blk_count; ++k)
            {
                const bm::word_t* arg_blk = ar_->v_arg_blk[k];
                if (!(mask & arg_blk[nword]))
                {
                    blk[nword] = 0;
                    return 0;
                }
            } // for k
            break;
        }
    } // for k

    for (; k < arg_blk_count; ++k)
    {
        if (ar_->v_arg_blk[k] == FULL_BLOCK_REAL_ADDR)
            continue;
        digest = bm::bit_block_and(blk, ar_->v_arg_blk[k], digest);
        if (!digest) // all zero
            return digest;
    } // for k
    return digest;
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::digest_type
aggregator<BV>::process_bit_blocks_sub(unsigned   arg_blk_count,
                                       digest_type digest)
{
    bm::word_t* blk = ar_->tb1;
    unsigned single_bit_idx;
    for (unsigned k = 0; k < arg_blk_count; ++k)
    {
        if (ar_->v_arg_blk[k] == FULL_BLOCK_REAL_ADDR) // golden block
        {
            digest = 0;
            break;
        }
        digest = bm::bit_block_sub(blk, ar_->v_arg_blk[k], digest);
        if (!digest) // all zero
            break;
        
        bool found = bm::bit_find_first_if_1(blk, &single_bit_idx, digest);
        if (found)
        {
            unsigned nword = unsigned(single_bit_idx >> bm::set_word_shift);
            unsigned mask = 1u << (single_bit_idx & bm::set_word_mask);
            for (++k; k < arg_blk_count; ++k)
            {
                const bm::word_t* arg_blk = ar_->v_arg_blk[k];
                if ((mask & arg_blk[nword]))
                {
                    blk[nword] = 0;
                    return 0;
                }
            } // for k
            break;
        }
    } // for k
    return digest;
}

// ------------------------------------------------------------------------

template<typename BV>
unsigned aggregator<BV>::resize_target(bvector_type& bv_target,
                            const bvector_type_const_ptr* bv_src, unsigned src_size,
                            bool init_clear)
{
    typename bvector_type::blocks_manager_type& bman_target = bv_target.get_blocks_manager();
    if (init_clear)
    {
        if (!bman_target.is_init())
            bman_target.init_tree();
        else
            bv_target.clear();
    }
    
    unsigned top_blocks = bman_target.top_block_size();
    auto size = bv_target.size();

    // pre-scan to do target size harmonization
    for (unsigned i = 0; i < src_size; ++i)
    {
        const bvector_type* bv = bv_src[i];
        BM_ASSERT(bv);
        const typename bvector_type::blocks_manager_type& bman_arg = bv->get_blocks_manager();
        unsigned arg_top_blocks = bman_arg.top_block_size();
        if (arg_top_blocks > top_blocks)
            top_blocks = bman_target.reserve_top_blocks(arg_top_blocks);
        auto arg_size = bv->size();
        if (arg_size > size)
        {
            bv_target.resize(arg_size);
            size = arg_size;
        }
    } // for i
    return top_blocks;
}

// ------------------------------------------------------------------------

template<typename BV>
bm::word_t* aggregator<BV>::sort_input_blocks_or(const bvector_type_const_ptr* bv_src,
                                                 unsigned src_size,
                                                 unsigned i, unsigned j,
                                                 unsigned* arg_blk_count,
                                                 unsigned* arg_blk_gap_count)
{
    bm::word_t* blk = 0;
    for (unsigned k = 0; k < src_size; ++k)
    {
        const bvector_type* bv = bv_src[k];
        BM_ASSERT(bv);
        const blocks_manager_type& bman_arg = bv->get_blocks_manager();
        const bm::word_t* arg_blk = bman_arg.get_block_ptr(i, j);
        if (!arg_blk)
            continue;
        if (BM_IS_GAP(arg_blk))
        {
            ar_->v_arg_blk_gap[*arg_blk_gap_count] = BMGAP_PTR(arg_blk);
            (*arg_blk_gap_count)++;
        }
        else // FULL or bit block
        {
            if (IS_FULL_BLOCK(arg_blk))
            {
                blk = FULL_BLOCK_FAKE_ADDR;
                *arg_blk_gap_count = *arg_blk_count = 0; // nothing to do
                break;
            }
            ar_->v_arg_blk[*arg_blk_count] = arg_blk;
            (*arg_blk_count)++;
        }
    } // for k
    return blk;
}

// ------------------------------------------------------------------------

template<typename BV>
bm::word_t* aggregator<BV>::sort_input_blocks_and(const bvector_type_const_ptr* bv_src,
                                                  unsigned src_size,
                                                  unsigned i, unsigned j,
                                                  unsigned* arg_blk_count,
                                                  unsigned* arg_blk_gap_count)
{
    bm::word_t* blk = FULL_BLOCK_FAKE_ADDR;
    for (unsigned k = 0; k < src_size; ++k)
    {
        const bvector_type* bv = bv_src[k];
        BM_ASSERT(bv);
        const typename bvector_type::blocks_manager_type& bman_arg = bv->get_blocks_manager();
        const bm::word_t* arg_blk = bman_arg.get_block_ptr(i, j);
        if (!arg_blk)
        {
            blk = 0;
            *arg_blk_gap_count = *arg_blk_count = 0; // nothing to do
            break;
        }
        if (BM_IS_GAP(arg_blk))
        {
            ar_->v_arg_blk_gap[*arg_blk_gap_count] = BMGAP_PTR(arg_blk);
            (*arg_blk_gap_count)++;
        }
        else // FULL or bit block
        {
            ar_->v_arg_blk[*arg_blk_count] =
                    IS_FULL_BLOCK(arg_blk) ? FULL_BLOCK_REAL_ADDR: arg_blk;
            (*arg_blk_count)++;
        }
    } // for k
    return blk;
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_or_horizontal(bvector_type& bv_target,
                     const bvector_type_const_ptr* bv_src, unsigned src_size)
{
    BM_ASSERT(src_size);
    if (src_size == 0)
    {
        bv_target.clear();
        return;
    }

    const bvector_type* bv = bv_src[0];
    bv_target = *bv;
    
    for (unsigned i = 1; i < src_size; ++i)
    {
        bv = bv_src[i];
        BM_ASSERT(bv);
        bv_target.bit_or(*bv);
    }
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_and_horizontal(bvector_type& bv_target,
                     const bvector_type_const_ptr* bv_src, unsigned src_size)
{
    BM_ASSERT(src_size);
    
    if (src_size == 0)
    {
        bv_target.clear();
        return;
    }
    const bvector_type* bv = bv_src[0];
    bv_target = *bv;
    
    for (unsigned i = 1; i < src_size; ++i)
    {
        bv = bv_src[i];
        BM_ASSERT(bv);
        bv_target.bit_and(*bv);
    }
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::combine_and_sub_horizontal(bvector_type& bv_target,
                                                const bvector_type_const_ptr* bv_src_and,
                                                unsigned src_and_size,
                                                const bvector_type_const_ptr* bv_src_sub,
                                                unsigned src_sub_size)
{
    BM_ASSERT(src_and_size);

    combine_and_horizontal(bv_target, bv_src_and, src_and_size);

    for (unsigned i = 0; i < src_sub_size; ++i)
    {
        const bvector_type* bv = bv_src_sub[i];
        BM_ASSERT(bv);
        bv_target -= *bv;
    }
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::prepare_shift_right_and(bvector_type& bv_target,
                                   const bvector_type_const_ptr* bv_src,
                                   unsigned src_size)
{
    top_block_size_ = resize_target(bv_target, bv_src, src_size);

    // set initial carry overs all to 0
    for (unsigned i = 0; i < src_size; ++i) // reset co flags
        ar_->carry_overs_[i] = 0;
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::combine_shift_right_and(
                bvector_type& bv_target,
                const bvector_type_const_ptr* bv_src_and, unsigned src_and_size,
                bool any)
{
    BM_ASSERT_THROW(src_size < max_aggregator_cap, BM_ERR_RANGE);
    if (!src_and_size)
    {
        bv_target.clear();
        return false;
    }
    prepare_shift_right_and(bv_target, bv_src_and, src_and_size);

    for (unsigned i = 0; i < bm::set_array_size; ++i)
    {
        if (i > top_block_size_)
        {
            if (!this->any_carry_overs(src_and_size))
                break; // quit early if there is nothing to carry on
        }

        unsigned j = 0;
        do
        {
            bool found = combine_shift_right_and(i, j, bv_target, bv_src_and, src_and_size);
            if (found && any)
                return found;
        } while (++j < bm::set_array_size);

    } // for i

    return bv_target.any();
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::combine_shift_right_and(unsigned i, unsigned j,
                                             bvector_type& bv_target,
                                        const bvector_type_const_ptr* bv_src,
                                        unsigned src_size)
{
    typename bvector_type::blocks_manager_type& bman_target =
                                                bv_target.get_blocks_manager();
    bm::word_t* blk = temp_blk_ ? temp_blk_ : ar_->tb1;
    unsigned char* carry_overs = &(ar_->carry_overs_[0]);

    bm::id64_t digest = ~0ull; // start with a full content digest

    // first initial block is just copied over from the first AND
    {
        const blocks_manager_type& bman_arg = bv_src[0]->get_blocks_manager();
        BM_ASSERT(bman_arg.is_init());
        const bm::word_t* arg_blk = bman_arg.get_block(i, j);
        if (BM_IS_GAP(arg_blk))
        {
            bm::bit_block_set(blk, 0);
            bm::gap_add_to_bitset(blk, BMGAP_PTR(arg_blk));
        }
        else
        {
            if (arg_blk)
                bm::bit_block_copy(blk, arg_blk);
            else
            {
                bm::bit_block_set(blk, 0);
                digest = 0;
            }
        }
        carry_overs[0] = 0;
    }
    
    unsigned k = 1;
    for (; k < src_size; ++k)
    {
        unsigned carry_over = carry_overs[k];
        if (!digest && !carry_over) // 0 into "00000" block >> 0
        {
            BM_ASSERT(bm::bit_is_all_zero(blk));
            continue;
        }
        
        const bm::word_t* arg_blk = get_arg_block(bv_src, k, i, j);
        carry_overs[k] = process_shift_right_and(arg_blk, digest, carry_over);
    } // for k
    
    // block now gets emitted into the target bit-vector
    if (digest)
    {
        BM_ASSERT(!bm::bit_is_all_zero(blk));
        bman_target.copy_bit_block(i, j, blk);
        return true;
    }
    return false;
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::process_shift_right_and(const bm::word_t* arg_blk,
                                             digest_type&      digest,
                                             unsigned          carry_over)
{
    bm::word_t* blk = temp_blk_ ? temp_blk_ : ar_->tb1;

    if (BM_IS_GAP(arg_blk)) // GAP argument
    {
        if (digest)
        {
            carry_over =
                bm::bit_block_shift_r1_and_unr(blk, carry_over,
                                            FULL_BLOCK_REAL_ADDR, &digest);
        }
        else // digest == 0, but carry_over can change that
        {
            blk[0] = carry_over;
            carry_over = 0;
            digest = blk[0]; // NOTE: this does NOT account for AND (yet)
        }
        bm::gap_and_to_bitset(blk, BMGAP_PTR(arg_blk), digest);
        digest = bm::update_block_digest0(blk, digest);
    }
    else // 2 bit-blocks
    {
        if (arg_blk) // use fast fused SHIFT-AND operation
        {
            if (digest)
            {
                carry_over =
                bm::bit_block_shift_r1_and_unr(blk, carry_over, arg_blk,
                                               &digest);
            }
            else // digest == 0
            {
                blk[0] = carry_over & arg_blk[0];
                carry_over = 0;
                digest = blk[0];
            }
        }
        else  // arg is zero - target block => zero
        {
            unsigned co = blk[bm::set_block_size-1] >> 31; // carry out
            if (digest)
            {
                bm::bit_block_set(blk, 0);  // TODO: digest based set
                digest ^= digest;
            }
            carry_over = co;
        }
    }
    return carry_over;
}

// ------------------------------------------------------------------------

template<typename BV>
const bm::word_t* aggregator<BV>::get_arg_block(
                                        const bvector_type_const_ptr* bv_src,
                                        unsigned k, unsigned i, unsigned j)
{
    const blocks_manager_type& bman_arg = bv_src[k]->get_blocks_manager();
    return bman_arg.get_block(i, j);
}

// ------------------------------------------------------------------------

template<typename BV>
bool aggregator<BV>::any_carry_overs(unsigned co_size) const
{
    for (unsigned i = 0; i < co_size; ++i)
        if (ar_->carry_overs_[i])
            return true;
    return false;
}

// ------------------------------------------------------------------------

template<typename BV>
void aggregator<BV>::stage(bm::word_t* temp_block)
{
    bvector_type* bv_target = check_create_target(); // create target vector
    BM_ASSERT(bv_target);
    
    temp_blk_ = temp_block;

    switch (operation_)
    {
    case BM_NOT_DEFINED:
        break;
    case BM_SHIFT_R_AND:
        prepare_shift_right_and(*bv_target, ar_->arg_bv0, arg_group0_size);
        operation_status_ = op_prepared;
        break;
    default:
        BM_ASSERT(0);
    } // switch
}

// ------------------------------------------------------------------------

template<typename BV>
typename aggregator<BV>::operation_status
aggregator<BV>::run_step(unsigned i, unsigned j)
{
    BM_ASSERT(operation_status_ == op_prepared || operation_status_ == op_in_progress);
    BM_ASSERT(j < bm::set_array_size);
    
    switch (operation_)
    {
    case BM_NOT_DEFINED:
        break;
        
    case BM_SHIFT_R_AND:
        {
        if (i > top_block_size_)
        {
            if (!this->any_carry_overs(arg_group0_size))
            {
                operation_status_ = op_done;
                return operation_status_;
            }
        }
        //bool found =
           this->combine_shift_right_and(i, j, *bv_target_,
                                        ar_->arg_bv0, arg_group0_size);
        operation_status_ = op_in_progress;
        }
        break;
    default:
        BM_ASSERT(0);
        break;
    } // switch
    
    return operation_status_;
}


// ------------------------------------------------------------------------


} // bm

#include "bmundef.h"

#endif
