/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// REQUIRES: binary_word_mask.cl

/**
 * @brief Declares a kernel for performing raster operations when a destination word
 *        maps to one source word (aligned scenario).
 *
 * The (rect|src|dst)_(width|height|offset|stride) parameters refer to words not pixels.
 * Each word represents a horizontal stripe of 32 pixels, with the most significant bit
 * representing the leftmost pixel. Zero bits correspond do white pixels.
 *
 * The (src|dst)_offset parameters account not just for padding of OpenCLGrid but also
 * for positions of the rectangle we are processing in source and destination image
 * coordinates. To be more precise, src[src_offset] is the word containing the top-left
 * pixel of the source rectangle. The same goes for dst_offset.
 *
 * The @p dst_left_edge_mask and @p dst_right_edge_mask define the bits we can touch
 * in the first and the last words on a destination line.
 */
#define BINARY_ROP_KERNEL_ALIGNED(kernel_name_, operation_)                            \
kernel void kernel_name_(                                                              \
    int const dst_rect_width, int const dst_rect_height,                               \
    global uint const* src, int const src_offset, int const src_stride,                \
    global uint* dst, int const dst_offset, int const dst_stride,                      \
    uint2 const edge_masks)                                                            \
{                                                                                      \
    int const global_word_x = get_global_id(0);                                        \
    int const global_word_y = get_global_id(1);                                        \
                                                                                       \
    if (global_word_x >= dst_rect_width | global_word_y >= dst_rect_height) {          \
        return;                                                                        \
    }                                                                                  \
                                                                                       \
    src += src_offset + mad24(global_word_y, src_stride, global_word_x);               \
    dst += dst_offset + mad24(global_word_y, dst_stride, global_word_x);               \
                                                                                       \
    uint const dst_word_mask = binary_word_mask(                                       \
        global_word_x, dst_rect_width, edge_masks);                                    \
                                                                                       \
    uint const old_dst_word = *dst;                                                    \
    *dst = bitselect(old_dst_word, operation_(*src, old_dst_word), dst_word_mask);     \
}

/**
 * @brief Declares a kernel for performing raster operations when
 *        dst_pixel_rect.left() % 32 < src_pixel_rect.left() % 32.
 *
 * Most of the things said about the aligned case above apply here as well.
 * In this case, a destination word dst[dst_offset + i] maps to a
 * combination of src[src_offset + i] and src[src_offset + i + 1].
 * To be more precise:
 * @code
 * int const src_word1_lshift = 32 - src_word2_rshift;
 * dst[dst_offset + i] = src[src_offset + i]     << src_word1_lshift |
 *                       src[src_offset + i + 1] >> src_word2_rshift;
 * @endcode
 * The code fragment above explains the @p src_word2_rshift parameter.
 *
 * The dimensions of @p src_cache are (get_local_size(0) + 1, get_local_size(1)) words.
 * Note that while source and destination rectangles have the same pixel width, they
 * may have a different word width.
 */
#define BINARY_ROP_KERNEL_DST_AHEAD_OF_SRC(kernel_name_, operation_)                     \
kernel void kernel_name_(                                                                \
    int const src_rect_width, int const dst_rect_width, int const dst_rect_height,       \
    global uint const* src, int const src_offset, int const src_stride,                  \
    global uint* dst, int const dst_offset, int const dst_stride,                        \
    uint2 const edge_masks, int const src_word2_rshift,                                  \
    local uint* src_cache, int const src_cache_stride)                                   \
{                                                                                        \
    int const global_word_x = get_global_id(0);                                          \
    int const global_word_y = get_global_id(1);                                          \
    int const local_word_x = get_local_id(0);                                            \
    int const local_word_y = get_local_id(1);                                            \
    int const last_local_word_x = min(                                                        \
        (int)get_local_size(0),                                                          \
        dst_rect_width - (global_word_x - local_word_x)                                  \
    ) - 1;                                                                               \
                                                                                         \
    bool const out_of_bounds =                                                           \
        global_word_x >= dst_rect_width | global_word_y >= dst_rect_height;              \
                                                                                         \
    src += src_offset + mad24(global_word_y, src_stride, global_word_x);                 \
    dst += dst_offset + mad24(global_word_y, dst_stride, global_word_x);                 \
    src_cache += mad24(local_word_y, src_cache_stride, local_word_x);                    \
                                                                                         \
    if (!out_of_bounds) {                                                                \
        /* Populate the cache, except for the last column. */                            \
        *src_cache = *src;                                                               \
                                                                                         \
        if (local_word_x == last_local_word_x & global_word_x + 1 != src_rect_width) {   \
            /* Populate the last column of the cache. */                                 \
            src_cache[1] = src[1];                                                       \
        }                                                                                \
    }                                                                                    \
                                                                                         \
    /* We are going to be reading a pair of words from src_cache, one of which was */    \
    /* written by a thread different from us. Therefore, we need a barrier. */           \
    barrier(CLK_LOCAL_MEM_FENCE);                                                        \
                                                                                         \
    if (out_of_bounds) {                                                                 \
        return;                                                                          \
    }                                                                                    \
                                                                                         \
    uint const src_word = src_cache[0] << (32 - src_word2_rshift) |                      \
                          src_cache[1] >> src_word2_rshift;                              \
                                                                                         \
    uint const dst_word_mask = binary_word_mask(                                         \
        global_word_x, dst_rect_width, edge_masks);                                      \
                                                                                         \
    uint const old_dst_word = *dst;                                                      \
    uint const new_dst_word = operation_(src_word, old_dst_word);                        \
    *dst = bitselect(old_dst_word, new_dst_word, dst_word_mask);                         \
}

/**
 * @brief Declares a kernel for performing raster operations when
 *        src_pixel_rect.left() % 32 < dst_pixel_rect.left() % 32.
 *
 * Most of the things said about the aligned case above apply here as well.
 * In this case, a destination word dst[dst_offset + i] maps to a
 * combination of src[src_offset + i - 1] and src[src_offset + i].
 * To be more precise:
 * @code
 * int const src_word1_rshift = 32 - src_word2_rshift;
 * dst[dst_offset + i] = src[src_offset + i - 1] << src_word1_lshift |
 *                       src[src_offset + i]     >> src_word2_rshift;
 * @endcode
 * The code fragment above explains the @p src_word2_rshift parameter.
 *
 * The dimensions of @p src_cache are (get_local_size(0) + 1, get_local_size(1)) words.
 */
#define BINARY_ROP_KERNEL_SRC_AHEAD_OF_DST(kernel_name_, operation_)                     \
kernel void kernel_name_(                                                                \
    int const dst_rect_width, int const dst_rect_height,                                 \
    global uint const* src, int const src_offset, int const src_stride,                  \
    global uint* dst, int const dst_offset, int const dst_stride,                        \
    uint2 const edge_masks, int const src_word2_rshift,                                  \
    local uint* src_cache, int const src_cache_stride)                                   \
{                                                                                        \
    int const global_word_x = get_global_id(0);                                          \
    int const global_word_y = get_global_id(1);                                          \
    int const local_word_x = get_local_id(0);                                            \
    int const local_word_y = get_local_id(1);                                            \
                                                                                         \
    bool const out_of_bounds =                                                           \
        global_word_x >= dst_rect_width | global_word_y >= dst_rect_height;              \
                                                                                         \
    src += src_offset + mad24(global_word_y, src_stride, global_word_x);                 \
    dst += dst_offset + mad24(global_word_y, dst_stride, global_word_x);                 \
    src_cache += mad24(local_word_y, src_cache_stride, local_word_x);                    \
                                                                                         \
    if (!out_of_bounds) {                                                                \
        if (local_word_x == 0 & global_word_x != 0) {                                    \
            /* Populate the first column of the cache. */                                \
            src_cache[0] = src[-1];                                                      \
        }                                                                                \
                                                                                         \
        /* Populate the rest of the cache. */                                            \
        src_cache[1] = src[0];                                                           \
    }                                                                                    \
                                                                                         \
    /* We are going to be reading a pair of words from src_cache, one of which was */    \
    /* written by a thread different from us. Therefore, we need a barrier. */           \
    barrier(CLK_LOCAL_MEM_FENCE);                                                        \
                                                                                         \
    if (out_of_bounds) {                                                                 \
        return;                                                                          \
    }                                                                                    \
                                                                                         \
    uint const src_word = src_cache[0] << (32 - src_word2_rshift) |                      \
                          src_cache[1] >> src_word2_rshift;                              \
                                                                                         \
    uint const dst_word_mask = binary_word_mask(                                         \
        global_word_x, dst_rect_width, edge_masks);                                      \
                                                                                         \
    uint const old_dst_word = *dst;                                                      \
    uint const new_dst_word = operation_(src_word, old_dst_word);                        \
    *dst = bitselect(old_dst_word, new_dst_word, dst_word_mask);                         \
}

#define BINARY_ROP_KERNEL(action) \
BINARY_ROP_KERNEL_ALIGNED(binary_rop_kernel_ ## action ## _aligned, binary_rop_ ## action) \
BINARY_ROP_KERNEL_DST_AHEAD_OF_SRC(binary_rop_kernel_ ## action ## _dst_src, binary_rop_ ## action) \
BINARY_ROP_KERNEL_SRC_AHEAD_OF_DST(binary_rop_kernel_ ## action ## _src_dst, binary_rop_ ## action)

uint binary_rop_src(uint src, uint dst)
{
	return src;
}
BINARY_ROP_KERNEL(src)

uint binary_rop_not_src(uint src, uint dst)
{
	return ~src;
}
BINARY_ROP_KERNEL(not_src)

uint binary_rop_dst_and_not_src(uint src, uint dst)
{
	return dst & ~src;
}
BINARY_ROP_KERNEL(dst_and_not_src)

uint binary_rop_and(uint src, uint dst)
{
	return src & dst;
}
BINARY_ROP_KERNEL(and)

uint binary_rop_or(uint src, uint dst)
{
	return src | dst;
}
BINARY_ROP_KERNEL(or)

uint binary_rop_xor(uint src, uint dst)
{
	return src ^ dst;
}
BINARY_ROP_KERNEL(xor)
