//
// Copyright (c) 2009 Advanced Micro Devices, Inc. All rights reserved.
//

#pragma OPENCL EXTENSION cl_amd_printf : enable
__kernel void
hello()
{
	/*
	Just a stub kernel. 
	*/
	
    size_t i =  get_global_id(0);
    size_t j =  get_global_id(1);
    if ( i== get_local_size(0) - 1 && j==get_local_size(1) -1 )
	{
       printf("Hello, I'm an OpenCl multi-core device. my position right now is x=%d, y=%d in the group x=%d y=%d\n", i,j, get_group_id(0), get_group_id(1));
	}   
    
}
