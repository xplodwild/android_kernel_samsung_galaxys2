/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Video4Linux API header for Samsung S5P TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S5P_TVOUT_V4L2_H_
#define __S5P_TVOUT_V4L2_H_ __FILE__

extern int s5p_tvout_v4l2_constructor(struct platform_device *pdev);
extern void s5p_tvout_v4l2_destructor(void);

/*      Pixel format FOURCC depth  Description  */
/* 12  Y/CbCr 4:2:0 64x32 macroblocks */
#define V4L2_PIX_FMT_NV12T    v4l2_fourcc('T', 'V', '1', '2')
#define V4L2_PIX_FMT_NV21T    v4l2_fourcc('T', 'V', '2', '1')

#endif /* __S5P_TVOUT_V4L2_H_ */
