//#define KA
#ifdef KA
#include "C:\Users\alyashev\alyashev_advtechmoreenvy\main\sw\advtech\tools\opencl_emu\clemu\clemu_opencl.h"
#else
#include "clemu_opencl.h"
#endif

#define pow(a,b) float2(pown((float2)a, (int)b)).x

//__kernel void
//hello()


bool IsVertexRelevant(float4 *verts);
bool AreCornersEqual(__global bool *activeVerts,int width,int centerX,int centerZ,int radius,bool desiredVal);
bool AreSidesEqual(__global bool *activeNodes,int chunkWidth,int centerX,int centerZ,int radius,bool desiredVal);
void CrossCull(int cellWidth,int chunkBlockWidth,int curDepth, float4 *normals, bool *activeNodes);
void HorizontalWorker(    int chunkBlockWidth,    int maxDepth,    float4 *normals,    bool *activeNodes);
void VerticalWorker(    int chunkBlockWidth,    int maxDepth,    float4 *normals,    bool *activeNodes);
typedef enum{
    v0 = 0,
    v1 = 1, 
    v2 = 2,
    v3 = 3,
    v4 = 4
} VERTEXENUM;
//VERTEXENUM For edge tests:
// +              v0 
// ^               |  
// |               |
// Z axis   v3----v4----v1
// |               |
// v               |
//                v2
//           <- X axis -> +

//VERTEXENUM For cross tests:
// +        v0----------v1
// ^               |  
// |               |
// Z axis         v4
// |               |
// v               |
//          v3----------v2
//           <- X axis -> +
//notice that even though int2 indexes the second value as y,(??)
//it still cooresponds with the value on the z axis

//threadpool tasks assuming pool width is 4
//     x
//   0 1 2 3 
//  0\ \ \ \horizontal workers
//  1\ \ \ \
//  2\ \ \ \
//z 3\ \ \ \
//  4/ / / /vertical workers
//  5/ / / /
//  6/ / / /
//  7/ / / /


    float4 *normals;
	/*bool activeVerts[] = {
		1,0,1,0,1,0,1,0,1,
		0,0,0,0,0,0,0,0,0,
		1,0,1,0,1,0,1,0,1,
		0,0,0,0,0,0,0,0,0,
		1,0,1,0,1,0,1,0,1,
		0,0,0,0,0,0,0,0,0,
		1,0,1,0,1,0,1,0,1,
		0,0,0,0,0,0,0,0,0,
		1,0,1,0,1,0,1,0,1
	};*/

	bool activeVerts[] = {
		1,0,1,0,1,0,1,0,1,
		0,1,0,1,0,1,0,1,0,
		1,0,1,0,1,0,1,0,1,
		0,1,0,1,0,1,0,1,0,
		1,0,1,0,1,0,1,0,1,
		0,1,0,1,0,1,0,1,0,
		1,0,1,0,1,0,1,0,1,
		0,1,0,1,0,1,0,1,0,
		1,0,1,0,1,0,1,0,1
	};

//clockwise everything
__Kernel(hello)
	__ArgNULL
{
	//setup the args here since this stupid arg shit wont work
    int chunkWidth = 8;
    int maxDepth = 0;
	if( get_global_id(0)==0 && get_global_id(1)==0){//xx
		normals = new float4[81];
		//activeVerts = new bool[81];
		for(int i=0; i<81; i++){
			normals[i] =0,0,0;
			//activeVerts[i] = true;
		}
		//activeVerts 
	}
    //end of setup code
           
        if( get_global_id(1) <= get_global_size(1)/2){//xx
			HorizontalWorker(chunkWidth, maxDepth, normals, activeVerts);
			return;
        }
		VerticalWorker(chunkWidth, maxDepth, normals, activeVerts);


		int size1= get_global_size(0);
		int size2= get_global_size(1);
		if(get_global_id(0) == get_global_size(0)-1 && get_global_id(1) == get_global_size(1)-1){


			for(int x=0; x<9; x++){
				for(int y=0; y<9; y++){
					char* c;
					itoa(activeVerts[x+y*9], c, 10);
					printf(c);
				}
				printf("\n");
			}
		}

		int f=4;
		__Return;
    }

void VerticalWorker(
    int chunkBlockWidth,
    int maxDepth,
    float4 *normals,
    bool *activeNodes){
        //generate relevant information
        int chunkVertWidth = chunkBlockWidth+1;
        int z_id = get_global_id(0);////
        int x_id = get_global_id(1)-get_global_size(1)/2;////
		for(int curDepth=0; curDepth <= maxDepth; curDepth++){
			int curCellWidth = curDepth*2+2;
			int startPoint = curCellWidth/2;
			int step = curCellWidth;
			
			int pointX = x_id*step+startPoint;////
			int pointZ = z_id*step+curCellWidth;////
			
			//check if vertical removal is even valid
			//make sure each corner is inactive(false)
			bool canSetNode = true;
			if(curDepth != 0){
				canSetNode = AreCornersEqual(
				activeNodes,
				chunkVertWidth,
				pointX,
				pointZ,
				curDepth,
				false
				);
			}

			if( canSetNode){
				//now see if we can disable this node
				float4 verts[5];
				verts[v0] = normals[chunkVertWidth*pointX + pointZ+step/2];
				verts[v1] = normals[chunkVertWidth*(pointX+step/2) + pointZ];
				verts[v2] = normals[chunkVertWidth*pointX + pointZ-step/2];
				verts[v3] = normals[chunkVertWidth*(pointX-step/2) + pointZ];
				verts[v4] = normals[chunkVertWidth*pointX + pointZ];

				if(!IsVertexRelevant(verts)){
					activeNodes[chunkVertWidth*pointX + pointZ] = false;        
				}
			}
			//wait until all orthogonal culling is completed
			barrier(CLK_GLOBAL_MEM_FENCE);
			//figure out if this thread is going to do cross culling
			//if not, waits at the next fence like a good little worker
			CrossCull(
				curCellWidth,
				chunkBlockWidth,
				curDepth,
				normals,
				activeNodes
				);
			barrier(CLK_GLOBAL_MEM_FENCE);

			//see if this worker needs to be culled
			int numZWorkers = chunkBlockWidth/(curCellWidth*2)-1;
			int numXWorkers = chunkBlockWidth/(curCellWidth*2);

			if(x_id >= numXWorkers)
				break;
			if(z_id >= numZWorkers)
				break;
        }
        //int xThreads = chunkWidth/2-1;
        //int zThreads = chunkWidth/2;


}

void HorizontalWorker(
    int chunkBlockWidth,
    int maxDepth,
    float4 *normals,
    bool *activeNodes){
        //generate relevant information
        int chunkVertWidth = chunkBlockWidth+1;
        int x_id = get_global_id(0);
        int z_id = get_global_id(1);
        
		for(int curDepth=0; curDepth <= maxDepth; curDepth++){
			int curCellWidth = curDepth*2+2;
			int startPoint = curCellWidth/2;
			int step = curCellWidth;
			
			int pointX = x_id*step+curCellWidth;
			int pointZ = z_id*step+startPoint;//+curCellWidth;
			
			//check if horizontal removal is even valid
			//make sure each corner is inactive(false)
			bool canSetNode = true;
			if(curDepth != 0){
				canSetNode = AreCornersEqual(
				activeNodes,
				chunkVertWidth,
				pointX,
				pointZ,
				curDepth,
				false
				);
			}

			if( canSetNode){
				//now see if we can disable this node
				float4 verts[5];
				verts[v0] = normals[chunkVertWidth*pointX + pointZ+step/2];
				verts[v1] = normals[chunkVertWidth*(pointX+step/2) + pointZ];
				verts[v2] = normals[chunkVertWidth*pointX + pointZ-step/2];
				verts[v3] = normals[chunkVertWidth*(pointX-step/2) + pointZ];
				verts[v4] = normals[chunkVertWidth*pointX + pointZ];

				if(!IsVertexRelevant(verts)){
					activeNodes[chunkVertWidth*pointX + pointZ] = false;        
				}
			}
			//wait until all orthogonal culling is completed
			barrier(CLK_GLOBAL_MEM_FENCE);
			//figure out if this thread is going to do cross culling
			//if not, waits at the next fence like a good little worker
			CrossCull(
				curCellWidth,
				chunkBlockWidth,
				curDepth,
				normals,
				activeNodes
				);
			barrier(CLK_GLOBAL_MEM_FENCE);

			//see if this worker needs to be culled
			int numXWorkers = chunkBlockWidth/(curCellWidth*2)-1;
			int numZWorkers = chunkBlockWidth/(curCellWidth*2);

			if(x_id >= numXWorkers)
				break;
			if(z_id >= numZWorkers)
				break;
        }
        //int xThreads = chunkWidth/2-1;
        //int zThreads = chunkWidth/2;
    }
    
void CrossCull(
    int cellWidth,
    int chunkBlockWidth,
    int curDepth,
    float4 *normals,
    bool *activeNodes){
    
        int x_id = get_global_id(0);
        int z_id = get_global_id(1);
        
        int x_max = get_global_size(0);
        int z_max = get_global_size(1);

        //this enumerates the 2d array of worker ids into a 1d array of super_ids
        int super_id = x_id+z_id*x_max;
        int numCells = chunkBlockWidth/cellWidth;
        
        //to figure out whether or not this worker should do the crosscull, 
        //we see if its super_id would fit in a 1d array enumerated from [numCells, numCells]
        if(super_id/numCells >= numCells){
            //this worker doesnt participate in crossculling
            return;
        }
        
		//these coorespond to which cell this worker is going to try to cull
        int x_cell = super_id/numCells;
        int z_cell = super_id - x_cell*numCells;
		int x_vert = x_cell * cellWidth + 1;
		int z_vert = z_cell * cellWidth + 1;
        
		//make sure we're able to cull the cross
		//first check outer corners. They must be enabled.
        if( !AreCornersEqual(
			activeNodes,
            chunkBlockWidth+1,
            x_vert,
            z_vert,
            pow(2,curDepth),//cellwidth/2?
            true
            )){
                return;
        }
		//check the outer "sides"; they should be disabled for the cross
		//to be removed properly
		if( !AreSidesEqual(
           activeNodes,
            chunkBlockWidth+1,
            x_vert,
            z_vert,
            pow(2,curDepth),
            false
            )){
                return;
        }
		
		//now check inset corners, we want them to be disabled
		for(int depth = curDepth-1; depth>=0; depth--){
			if( !AreCornersEqual(
				activeNodes,
				chunkBlockWidth+1,
				x_vert,
				z_vert,
				pow(2,depth),
				false
				)){
					return;
			}
		}
		
		//okay, at this point we know that a cross cull would be valid
		//check to see if it's necessary now
		float4 verts[5];
		int chunkVertWidth = chunkBlockWidth+1;
        verts[v0] = normals[chunkVertWidth*(x_vert-cellWidth/2) + z_vert+cellWidth/2];
        verts[v1] = normals[chunkVertWidth*(x_vert+cellWidth/2) + z_vert+cellWidth/2];
        verts[v2] = normals[chunkVertWidth*(x_vert-cellWidth/2) + z_vert-cellWidth/2];
        verts[v3] = normals[chunkVertWidth*(x_vert+cellWidth/2) + z_vert-cellWidth/2];
        verts[v4] = normals[chunkVertWidth*x_vert + z_vert];

		if(!IsVertexRelevant(verts)){
			activeNodes[x_vert*chunkVertWidth+z_vert] = false;		
		}
		return;
    }

bool AreCornersEqual(
    __global bool *activeNodes,
    int chunkVertWidth,
    int centerX,
    int centerZ,
    int radius,
    bool desiredVal){
        
        bool ret=true;
		//xx all these gotos are probably going to cause issues with
		//irreducible control flow, try testing to see if the compiler's
		//optimizations with flag perform better than goto
        for(int x=centerX-radius; x<=centerX+radius; x+=radius*2){
            for(int z=centerZ-radius; z<=centerZ+radius; z+=radius*2){
                if( activeNodes[x*chunkVertWidth+z] != desiredVal ){
                    ret = false;
                    goto brkLoop;
                }
            }
        }
        brkLoop:
        return ret;
    }
    
bool AreSidesEqual(
    __global bool *activeNodes,
    int chunkWidth,
    int centerX,
    int centerZ,
    int radius,
    bool desiredVal){
        
        bool ret=true;
        //it would have probably been a better idea to just hardcode these loops
        for(int x=centerX-radius; x<=centerX+radius; x+=radius*2){
            if( activeNodes[x*chunkWidth+centerZ] != desiredVal ){
                ret = false;
                goto brkLoop;
            }
        }
        for(int z=centerZ-radius; z<=centerZ+radius; z+=radius*2){
            if( activeNodes[centerX*chunkWidth+z] != desiredVal ){
                ret = false;
                goto brkLoop;
            }
        }
        brkLoop:
        return ret;
    }

inline float Magnitude(float4 vec){
    return distance( (0,0,0), vec);
}

bool IsVertexRelevant(float4 *verts){
/*
    float angles[4];
    
    for(int i=0; i<4; i++){
        angles[i] = acos( 
            dot(verts[4], verts[i]) / 
            (Magnitude(verts[4]) * Magnitude(verts[i]))
        );
    }
    */
    bool disableCentNode = true;/*
    const float minAngle = 10 * 0.174533f;
    for(int i=0; i<4; i++){
        if(angles[i] > minAngle){
            disableCentNode = false;
            break;
        }
    }*/
    return !disableCentNode;
}



        //vertIdx[v0] = GetVertIndex(center.x-radius, center.y+radius, chunkWidth);
        //vertIdx[v1] = GetVertIndex(center.x+radius, center.y+radius, chunkWidth);
        //vertIdx[v2] = GetVertIndex(center.x+radius, center.y-radius, chunkWidth);
        //vertIdx[v3] = GetVertIndex(center.x-radius, center.y-radius, chunkWidth);
        //vertIdx[v4] = GetVertIndex(center.x, center.y, chunkWidth);

        //int vertIdx[5];
          //      vertIdx[v0] = GetVertIndex(center.x, center.y+radius, chunkWidth);
       // vertIdx[v1] = GetVertIndex(center.x+radius, center.y, chunkWidth);
       // vertIdx[v2] = GetVertIndex(center.x, center.y-radius, chunkWidth);
      //  vertIdx[v3] = GetVertIndex(center.x-radius, center.y, chunkWidth);
     //   vertIdx[v4] = GetVertIndex(center.x, center.y, chunkWidth);

