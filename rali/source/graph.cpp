#include <iostream>
#include <vx_ext_amd.h>

#include "graph.h"
#include "commons.h"

AgoTargetAffinityInfo
get_ago_affinity_info(
        RaliAffinity rali_affinity,
        int cpu_id,
        int gpu_id)
{
    AgoTargetAffinityInfo affinity;
    switch(rali_affinity) {
        case RaliAffinity::GPU:
            affinity.device_type =  AGO_TARGET_AFFINITY_GPU;
            affinity.device_info = (gpu_id >=0 && gpu_id <=9)? gpu_id : 0;
            break;
        case RaliAffinity::CPU:
            affinity.device_type = AGO_TARGET_AFFINITY_CPU;
            affinity.device_info = (cpu_id >=0 && cpu_id <=9)? cpu_id : 0;
            break;
        default:
            throw std::invalid_argument("Unsupported affinity");
    }
    return affinity;
}

Graph::Graph(vx_context context, RaliAffinity affinity, int cpu_id, int gpu_id):
_affinity(affinity),
_gpu_id(gpu_id),
_cpu_id(cpu_id),
_context(context)
{
    try
    {
        vx_status status;
        _mem_type = (_affinity == RaliAffinity::GPU) ? RaliMemType::OCL : RaliMemType::HOST;
        auto vx_affinity = get_ago_affinity_info(_affinity , cpu_id, gpu_id);

        _graph = vxCreateGraph(_context);

        if((status = vxGetStatus((vx_reference)_graph)) != VX_SUCCESS)
            THROW("vxCreateGraph failed " + TOSTR(status))


        // Setting attribute to run on CPU or GPU
        if((status = vxSetGraphAttribute(   _graph,
                                            VX_GRAPH_ATTRIBUTE_AMD_AFFINITY,
                                            &vx_affinity,
                                            sizeof(vx_affinity))) != VX_SUCCESS)
            THROW("vxSetGraphAttribute failed " + TOSTR(status))

    }
    catch(const std::exception& e)
    {
        release();
        throw;
    }

}

Graph::Status
Graph::verify()
{
    vx_status status;
    if((status = vxVerifyGraph(_graph)) != VX_SUCCESS) 
        THROW("vxVerifyGraph failed " + TOSTR(status))

    return Status::OK;    
}

Graph::Status
Graph::process()
{
    vx_status status;
    if((status = vxProcessGraph(_graph)) != VX_SUCCESS) 
         THROW("ERROR: vxProcessGraph failed " + TOSTR(status))

    return  Status::OK;    
}

Graph::Status
Graph::release()
{
    vx_status status = VX_SUCCESS;

    if(_graph && (status = vxReleaseGraph(&_graph)) != VX_SUCCESS) 
        LOG ("Failed to call vxReleaseGraph " + TOSTR(status))

    return  Status::OK;  
}

