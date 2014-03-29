#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdint.h>
#include <new>

//Define a structure that contains the "rules" for the game
struct Ruleset {
	int loadtime,
	turntime,
	rows,
	cols,
	turns,
	viewradius2,
	attackradius2,
	spawnradius2;
	//int64_t is a type declared in stdint.h to explicitly allow 64bit ints
	int64_t player_seed;
};

//A class that implements dynamic arrays
class TwoD{
	public:
		int sizex, sizey;
		unsigned char *map;
		//Constructor that creates a dynamic array of desired x,y dimensions and initializes it
		TwoD(int x, int y, unsigned char init){
			sizex=x;
			sizey=y;
			map = new (std::nothrow) unsigned char [x*y];
			if (map==0)
				std::cerr<<"[JBot] Unable to allocate new 2d array requesting "<<(x*y)<<" squares"<<std::endl;
			for (int i=0; i<(x*y); i++){
				map[i] = init;
			}
		}

		//Destructor that frees up the space that was allocated to myarray when the object was born
		~TwoD(){
			delete [] map;
		}
};

//Global variables: 'turn' is the turn number as reported from ReadIn(), 'ongoing' reports game life changing to 0 if "end" is read by ReadIn()
int turn=0, ongoing=1;
Ruleset gamerules;
//Create map feature maps of the appropriate size
TwoD terrain(gamerules.cols, gamerules.rows, 1),
	 food(gamerules.cols, gamerules.rows, 0),
	 hill(gamerules.cols, gamerules.rows, 0),
	 ant(gamerules.cols, gamerules.rows, 0);

//Read in game initial conditions into our Ruleset structure: ruleset
Ruleset GetRules() {
	std::string label, extra;
	Ruleset ruleset;

	//Parse and ignore all text until "turn 0"
	while(std::cin >>label>>extra){
	label += extra;
	if(label=="turn0")
		break;
	}

	//Parse in a ruleset label, decide where in the Ruleset struct it belongs and then read in the value, do this until we see "ready"
	while(std::cin >> label){
		if(label=="loadtime")
			std::cin >> ruleset.loadtime;
		else if(label=="turntime")
			std::cin >> ruleset.turntime;
		else if(label=="rows")
			std::cin >> ruleset.rows;
		else if(label=="cols")
			std::cin >> ruleset.cols;
		else if(label=="turns")
			std::cin >> ruleset.turns;
		else if(label=="viewradius2")
			std::cin >> ruleset.viewradius2;
		else if(label=="attackradius2")
			std::cin >> ruleset.attackradius2;
		else if(label=="spawnradius2")
			std::cin >> ruleset.spawnradius2;
		else if(label=="player_seed")
			std::cin >> ruleset.player_seed;
		else if(label=="ready"){
			std::cerr<<"[JBot]Ruleset Loaded"<<std::endl;
			break;
		}
		//Any labels that are unknown are skipped and spit out here, along with their values (if any)
		else
			std::cerr<<"[JBot]Unknown tag: "<<label<<std::endl;
	}
	return ruleset;
}

int ReadIn(){
	int x, y, owner;
	std::string input;
	while(std::cin >> input){
		if(input=="w"){
			std::cin>>y;
			std::cin>>x;
			terrain.map[(y*terrain.sizex)+x]=0;
		}
		else if(input=="f"){
			std::cin>>y;
			std::cin>>x;
			food.map[(y*terrain.sizex)+x]=1;
		}
		else if(input=="h"){
			std::cin>>y;
			std::cin>>x;
			std::cin>>owner;
			hill.map[(y*terrain.sizex)+x]=(owner+1);
		}
		else if(input=="a"){
			std::cin>>y;
			std::cin>>x;
			std::cin>>owner;
			ant.map[(y*terrain.sizex)+x]=(owner+1);
		}
		else if(input=="turn"){
			std::cin>>turn;
		}
		else if(input=="go"){
			break;
		}
		//Any labels that are unknown are skipped and spit out here, along with their values (if any)
		else
			std::cerr<<"[JBot]Unknown input: "<<input<<std::endl;
	}
	return
}

int main()
{
	//Read in the gamerules
	gamerules = GetRules();
	//Tell the engine we are done initializing
	std::cout<<"go"<<std::endl;

	return 0;
}