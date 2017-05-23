/* pocl_img_buf_cpy.c: common parts of image and buffer copying

   Copyright (c) 2011 Universidad Rey Juan Carlos
   Copyright (c) 2015 Giuseppe Bilotta
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "pocl_cl.h"
#include <assert.h>
#include "pocl_image_util.h"
#include "pocl_util.h"

/* Copies between images and rectangular buffer copies share most of the code,
   with specializations only needed for specific checks. The actual API calls
   thus defer to this function, with the additional information of which of src
   and/or dst is an image and which is a buffer.
 */

cl_int pocl_rect_copy(cl_command_queue command_queue,
                      cl_command_type command_type,
                      cl_mem src,
                      cl_int src_is_image,
                      cl_mem dst,
                      cl_int dst_is_image,
                      const size_t *src_origin,
                      const size_t *dst_origin,
                      const size_t *region,
                      size_t src_row_pitch,
                      size_t src_slice_pitch,
                      size_t dst_row_pitch,
                      size_t dst_slice_pitch,
                      cl_uint num_events_in_wait_list,
                      const cl_event *event_wait_list,
                      cl_event *event)
{
  cl_int errcode;
  cl_device_id device;
  _cl_command_node *cmd = NULL;
  unsigned i;
  cl_mem buffers[2] = {src, dst};

  POCL_RETURN_ERROR_COND((command_queue == NULL), CL_INVALID_COMMAND_QUEUE);

  if (src_is_image || dst_is_image)
    {
      POCL_RETURN_ERROR_ON((!command_queue->device->image_support), CL_INVALID_OPERATION,
        "Device %s does not support images\n", command_queue->device->long_name);
    }

  POCL_RETURN_ERROR_COND((src == NULL), CL_INVALID_MEM_OBJECT);
  POCL_RETURN_ERROR_COND((dst == NULL), CL_INVALID_MEM_OBJECT);
  POCL_RETURN_ERROR_COND((src_origin == NULL), CL_INVALID_VALUE);
  POCL_RETURN_ERROR_COND((dst_origin == NULL), CL_INVALID_VALUE);
  POCL_RETURN_ERROR_COND((region == NULL), CL_INVALID_VALUE);

  if (src_is_image)
    {
      POCL_RETURN_ERROR_ON((!src->is_image),
        CL_INVALID_MEM_OBJECT, "src_image is not an image\n");
      POCL_RETURN_ERROR_ON((src->type == CL_MEM_OBJECT_IMAGE2D && src_origin[2] != 0),
        CL_INVALID_VALUE, "src_origin[2] must be 0 for 2D src_image\n");
      errcode = pocl_check_device_supports_image(src, command_queue);
      if (errcode != CL_SUCCESS)
        return errcode;
    }
  else
    {
      POCL_RETURN_ERROR_ON((src->type != CL_MEM_OBJECT_BUFFER),
        CL_INVALID_MEM_OBJECT, "src is not a CL_MEM_OBJECT_BUFFER\n");
    }

  if (dst_is_image)
    {
      POCL_RETURN_ERROR_ON((!dst->is_image),
        CL_INVALID_MEM_OBJECT, "dst is not an image\n");
      POCL_RETURN_ERROR_ON((dst->type == CL_MEM_OBJECT_IMAGE2D && dst_origin[2] != 0),
        CL_INVALID_VALUE, "dst_origin[2] must be 0 for 2D dst_image\n");
      errcode = pocl_check_device_supports_image(dst, command_queue);
      if (errcode != CL_SUCCESS)
        return errcode;
    }
  else
    {
      POCL_RETURN_ERROR_ON((dst->type != CL_MEM_OBJECT_BUFFER),
        CL_INVALID_MEM_OBJECT, "dst is not a CL_MEM_OBJECT_BUFFER\n");
    }

  if (src_is_image && dst_is_image)
    {
      POCL_RETURN_ERROR_ON((src->image_channel_order != dst->image_channel_order),
        CL_IMAGE_FORMAT_MISMATCH, "src and dst have different image channel order\n");
      POCL_RETURN_ERROR_ON((src->image_channel_data_type != dst->image_channel_data_type),
        CL_IMAGE_FORMAT_MISMATCH, "src and dst have different image channel data type\n");
      POCL_RETURN_ERROR_ON((
          (dst->type == CL_MEM_OBJECT_IMAGE2D || src->type == CL_MEM_OBJECT_IMAGE2D) &&
          region[2] != 1),
        CL_INVALID_VALUE, "for any 2D image copy, region[2] must be 1\n");
   }


  /* Images need to recompute the regions in bytes before copying */
  size_t mod_region[3], mod_src_origin[3], mod_dst_origin[3];
  memcpy(mod_region, region, 3*sizeof(size_t));
  memcpy(mod_src_origin, src_origin, 3*sizeof(size_t));
  memcpy(mod_dst_origin, dst_origin, 3*sizeof(size_t));

  if (src_is_image)
    {
      mod_region[0] *= src->image_elem_size * src->image_channels;
      mod_src_origin[0] *= src->image_elem_size * src->image_channels;
    }
  if (dst_is_image)
    {
      if (!src_is_image)
        mod_region[0] *= dst->image_elem_size * dst->image_channels;
      mod_dst_origin[0] *= dst->image_elem_size * dst->image_channels;
    }

  if (src_is_image)
    {
      src_row_pitch = src->image_row_pitch;
      src_slice_pitch = src->image_slice_pitch;
    }

  if (dst_is_image)
    {
      dst_row_pitch = dst->image_row_pitch;
      dst_slice_pitch = dst->image_slice_pitch;
    }

  POCL_RETURN_ERROR_ON(((command_queue->context != src->context)
      || (command_queue->context != dst->context)),
    CL_INVALID_CONTEXT,
      "src, dst and command_queue are not from the same context\n");

  POCL_RETURN_ERROR_COND((event_wait_list == NULL && num_events_in_wait_list > 0),
    CL_INVALID_EVENT_WAIT_LIST);

  POCL_RETURN_ERROR_COND((event_wait_list != NULL && num_events_in_wait_list == 0),
    CL_INVALID_EVENT_WAIT_LIST);

  size_t region_bytes = mod_region[0] * mod_region[1] * mod_region[2];
  POCL_RETURN_ERROR_ON((region_bytes <= 0), CL_INVALID_VALUE, "All items in region must be >0\n");

  if (pocl_buffer_boundcheck_3d(src->size, mod_src_origin, mod_region,
      &src_row_pitch, &src_slice_pitch, "src_") != CL_SUCCESS)
    return CL_INVALID_VALUE;

  if (pocl_buffer_boundcheck_3d(dst->size, mod_dst_origin, mod_region,
      &dst_row_pitch, &dst_slice_pitch, "dst_") != CL_SUCCESS)
    return CL_INVALID_VALUE;

  if (src == dst)
    {
      POCL_RETURN_ERROR_ON((src_slice_pitch != dst_slice_pitch),
        CL_INVALID_VALUE, "src and dst are the same object,"
        " but the given dst & src slice pitch differ\n");

      POCL_RETURN_ERROR_ON((src_row_pitch != dst_row_pitch),
        CL_INVALID_VALUE, "src and dst are the same object,"
        " but the given dst & src row pitch differ\n");

      POCL_RETURN_ERROR_ON(
        (check_copy_overlap(mod_src_origin, mod_dst_origin, mod_region,
          src_row_pitch, src_slice_pitch)),
        CL_MEM_COPY_OVERLAP, "src and dst are the same object,"
        "and source and destination regions overlap\n");

    }

  POCL_CHECK_DEV_IN_CMDQ;

  errcode = pocl_create_command (&cmd, command_queue, command_type,
                                 event, num_events_in_wait_list,
                                 event_wait_list, 2, buffers);

  if (errcode != CL_SUCCESS)
    return errcode;

  cmd->command.copy_image.src_buffer = src;
  cmd->command.copy_image.src_device =
    (src->owning_device) ? src->owning_device : command_queue->device;
  cmd->command.copy_image.dst_buffer = dst;
  cmd->command.copy_image.dst_device =
    (dst->owning_device) ? dst->owning_device : command_queue->device;
  cmd->command.copy_image.src_origin[0] = mod_src_origin[0];
  cmd->command.copy_image.src_origin[1] = mod_src_origin[1];
  cmd->command.copy_image.src_origin[2] = mod_src_origin[2];
  cmd->command.copy_image.dst_origin[0] = mod_dst_origin[0];
  cmd->command.copy_image.dst_origin[1] = mod_dst_origin[1];
  cmd->command.copy_image.dst_origin[2] = mod_dst_origin[2];
  cmd->command.copy_image.region[0] = mod_region[0];
  cmd->command.copy_image.region[1] = mod_region[1];
  cmd->command.copy_image.region[2] = mod_region[2];
  cmd->command.copy_image.src_rowpitch = src_row_pitch;
  cmd->command.copy_image.src_slicepitch = src_slice_pitch;
  cmd->command.copy_image.dst_rowpitch = dst_row_pitch;
  cmd->command.copy_image.dst_slicepitch = dst_slice_pitch;

  POname(clRetainMemObject) (src);
  POname(clRetainMemObject) (dst);

  pocl_command_enqueue(command_queue, cmd);

  return CL_SUCCESS;
}
