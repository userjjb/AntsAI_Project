//Testing tool used to examine behavior of Adaptive Convex Regioning algorithm with omniscient knowledge of a map
//By userjjb - Nov 2011

/**
Adaptive Convex Regioning attemps to provide a implementation specific paradigm for improved pathfinding in 2D maps where movement is 
only possible between adjacent squares. It splits the map into connected regions, distinguishing between "local" movement inside a region
and "global" movement between regions. A region is defined as a convex space of at least 2x2 dimension free of obstructions. All points 
within a convex space can be connected by a line that does not exit that region, so pathfinding within a region is trivial. 
The main benefits of ACR as a paradigm are:

1: Reduction of effective nodes in a pathfinding search space for global movement by conflation of "atomic" nodes into "meta-nodes" in 
the connectivity graph.
-For a given density and local gradient of obstructions in a map the average region area should remain nearly constant. So nodes will 
 still increase as the square of map side length and the O() time is only as good as the pathfinding algorithm, but we are putting a
 potentially *large* coefficient in front on the order of 1/AvgAreaOfRegion. For maps with low density or high local gradient for 
 obstructions the size of regions can be quite large. Obviously in maps with high density and homogenous distribution of obstructions
 we will realize little benefit and we would be better off with vanilla A*

2: Trivialization of local travel.
-Barring dynamic per turn fringe cases of ant-ant and ant-food collisions, all intra-region travel is trivial! No pathfinding is required,
 simply move in the direction of the desired destination.

3: A quantitative way to delineate what is "local" and "global" for position context sensitive actions. 
-Ants should mostly stay inside of a region because nearly all tasks only have local relevance (food gathering, combat, etc). Inter-region
 travel should only occur under two circumstances: a) Maintaining desired regional population (Whether this be populating new regions or
 replenishing lost ants and b) *High* priority tasks that require force unable to be performed by the local ant population (assaults on
 well defended hills, global map control, other fancy "high-level" tasks and multi-turn strategies)

4: A direct consequence of benefits 1-3; pathfinding inside a region is trivial, pathfinding between regions is 1 or 2 orders of magnitude
quicker, and even then only occurs infrequently. Taken all together this means that we almost never actually have to peform a pathfinding
algorithm pass!
-We should only have to perform a pathfinding routine (say A*) when a significant strategy is activated or a major event requires notable
 response. The bulk of even inter-region travel will only be to neighbor regions and is trivial as well.
 **/

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#define OOLOS 2
#define WATER 1
#define PASSABLE 0
#define MAXMAPROW 200
#define MAXMAPCOL 200
#define MAXCHILDREN 100

//Accomadate all map dimensions
int grid[MAXMAPROW][MAXMAPCOL];
//We treat hill 0 as "my" hill
int myhillx, myhilly, rows, cols, players;

//Grid initialization, all terrain is initially impassable and is out-of-LOS (OOLOS) to prevent having a halo of passable terrain around
//smaller maps, this might cause pathfinding to break
void InitializeGrid(){
	for(int x=0;x<MAXMAPROW;x++){
		for(int y=0;y<MAXMAPCOL;y++){
			grid[x][y]=OOLOS;
		}
	}
}

/** A class that implements region generation and storage. The root region is order "2" and is seeded on player 0's hill. Bi-directional
"feelers" are then sent out from the seed orthogonally. The combined length of both feelers must be at least 1, to create non-trivial 
regions. If they aren't the hill is in a nook and the seed is moved by 1 square in the direction of the furthest side along the short 
feeler to excape from the nook. Hills shouldn't be any farther in a nook/crevice otherwise the map should probably be illegal.

There is a good chance that a feeler extended "too" far and if we were to create the region with the width and length from each feeler we
would include some water along at least one of the edges. We could try to "inflate" the area organically, but this is likely expensive to 
do, and is non-trivial to choose which direction to expand in each step to prevent small protrusions from "catching" the inflating region 
and prematurely halting expansion. Instead we naively create the full region and then add up the sum of squares along a side. This MUST be 
0 otherwise it contains at least one water square (or a square claimed by another region). If an edge's sum is non-0 then we shrink it back. 
For many cases a side only needs to recede by 1 square to "trim the fuzz". If a side needs to recede more than one then we note this and 
recede it until it's free of water and note how much water surrounds the region. We then revert the shrinking and recede the perpendicular 
side of the quadrant that contains the obstructions and note the ratio of bordering water and keep whichever configuration has the highest 
ratio of bordering water in an attempt to choose the "best" looking shape.

In order to create new child regions we note sections of edges that are free of water perpendicular to the edge at least 2 squares out.
We plant new seeds at the center of these sections and grow new regions.

In conventional graph theory terms "vertices" are placed at the midpoint of a boundary between regions, with "edges" connecting each vertex
to all others internally within a region. Edge length is simply the Manhattan distance between vertices. For paths with 'U' shaped sections
the sum of the edge lengths for the path overestimates the actual optimal path distance because we can cut tighter 'U's then through a
boundary's midpoint**/

//Class that implements region creation and manipulation
//origin is the North-East corner
//We should try to make this map-knowledge agnostic so we can dynamicly expand regions/create children
class Region{
	unsigned char top,bot,left,right;
public:
	unsigned char origin,height,width;
	//This array stores all the child seeds. 100 *should* be the theoretical max since a 200 long region with 3 wide hallways with 1 tile walls
	unsigned char children[MAXCHILDREN][2];
	//Constructor, forms a region from a seed's coordinates and finds child seed sites
	Region(int x, int y, int order){
		int sourcex=x, sourcey=y, obstructions=0, looped=0;
		//Sends out feelers in cardinal directions until hits wall/LOS/region
		while(!grid[x][++y]){}
			bot = --y;
			y = sourcey;
		while(!grid[x][--y]){}
			top = ++y;
			y = sourcey;
		while(!grid[++x][y]){}
			right = --x;
			x = sourcex;
		while(!grid[--x][y]){}
			left = ++x;
		height=bot-top;
		width=right-left;
		//See if/how many obstructions the proto-region contains
		while (looped<5){
			for (int i=left;i<=right;i++){
				for (int j=top;j<=bot;j++){
					obstructions += grid[i][j]/grid[i][j];
				}
			}
			//If our proto-region is clear break early leaving feeler determined dimensions unchanged
			if(looped==0 && obstructions==0)
				break
		}

	}
};

int main() {
	string line;
	int leave=1, temp, prepend, length, y=0;

	InitializeGrid();
	//Open map file
	cout<<"Enter map file name (with extension)..."<<endl;
	cin>>line;
	ifstream myfile (line);
	if (myfile.is_open()){ 
	
		//Read in map header info
		while(leave&&(myfile >> line)){
			if (line=="rows")
				myfile>>rows;
			else if (line=="cols")
				myfile>>cols;
			else if (line=="players"){
				myfile>>players;
				leave=0;
			}
			else
				cerr<<"Unknown input: "<<line<<endl;
		}
		//Read in file ignoring line prefixes. Water is logically true, land and hills are treated as logically false. 
		//This is so squares in regions stored in grid[][] as their order number of that region 
		//are treated as impassable after creation for the purposes of creating new regions to avoid overlapping regions
		while ( myfile.good() ){
			getline (myfile,line);
			length = line.size();
			prepend = length-cols;
			if (length){	//Reject blank lines
			for (int x=prepend;x<length;x++){
				if (line[x]=='%')
					temp=WATER;
				else if (line[x]=='.')
					temp=PASSABLE;
				else if (line[x]=='0'){
					temp=PASSABLE;
					myhillx= (x-prepend);
					myhilly= (y);
					}
				else if (line[x]=='1'||line[x]=='2'||line[x]=='3'||line[x]=='4'||line[x]=='5'||line[x]=='6'||line[x]=='7'||line[x]=='8'||line[x]=='9')
					temp=PASSABLE;
				else
					cerr<<"Unknown map tile!"<<endl;

				grid[x-prepend][y]=temp;
			}
			y++;
			}
		}
		myfile.close();
	}
	else cout << "Unable to open file";

	//Region Generation
	Region Prime(myhillx,myhilly,3);
	
	//Draw the map on-screen
	for(int y=0;y<rows;y++)
	{
		for(int x=0;x<cols;x++)
		{
			cout<<((grid[x][y]));
		}
		cout<<endl;
	}
	cout<<"Enter any value to exit..."<<endl;
	cin>>line;
	return 0;
}