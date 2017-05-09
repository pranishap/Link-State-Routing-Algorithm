/*
 * LinkStateRoutingProtocol.c
 *
 *  Created on: Nov 13, 2015
 *      Author: pranisha
 */


#include <stdio.h>
#include <unistd.h>
#include <termios.h>


#define ROUTER_MAX 100
int sourceRouter = -1;
int destinationRouter = -1;
//Filename used for loading the data
char topologyFileName[30] ;
int totalRouter = 0;

/*
 * This structure is used to represent the shortest path to each router.
 *
 */
struct shortestPath
{
   //Total cost from the source
   int totalCost;
   //To check if the shortest path is found or not.
   int costFound;
   //router number of the previous router in the path
   int previousRouter;

};

/*
 * This structure is used to represent each router.
 *
 */
struct router
{
  //Represents the router number
  int routerNum;
  //Represents the cost of direct link to this router
  int directLink[ROUTER_MAX][ROUTER_MAX];
  //To check if the router was already checked for the path from source
  int sequenceNum[ROUTER_MAX];
  //Stores the shortest path to each router
  struct shortestPath path[ROUTER_MAX];
};

//List of routers which forms a network
struct router network[ROUTER_MAX];
//Empty router
static const struct router EmptyRouter;
//Matrix which is the original topology
int originalNetwork[ROUTER_MAX][ROUTER_MAX];
int emptyNetwork[ROUTER_MAX][ROUTER_MAX];

/*
 * This function reads the file of given file name and counts the total number of routers
 * and populates the originalNetwork array with values read from the file.
 *
 */
int getOriginalTopology(){
	  FILE *fh;
	  sourceRouter = -1;
	  destinationRouter = -1;
	  totalRouter = 0;
	  int i,j;
	  //clearing the original network matrix
	  for (i = 0; i < ROUTER_MAX; ++i) {
	  	  for (j = 0; j < ROUTER_MAX; ++j)
		   originalNetwork[i][j] = 0;
	  }
	  //open file
      if((fh = fopen(topologyFileName,"r"))==NULL)
	      return -1;

	  //Finding total number of routers
	  char ch;
	  int isChar = 0;
	  totalRouter = 0;
	  while((ch = fgetc(fh)) != '\n')
	  {
	    if(ch>='0' && ch<='9' && !isChar){
	      isChar = 1;
	      totalRouter++;
	    }else{
	      isChar =0;
	    }
	  }

	  fseek(fh, 0, SEEK_SET);
	  for (i = 0; i < totalRouter; ++i) {
	  for (j = 0; j < totalRouter; ++j)
		//adding values to the originalNetwork
	    fscanf (fh, "%d", &originalNetwork[i][j]);
	  }
	  fclose (fh);
      initNetwork();
      printTopologyMatrix();
      int node_index=0;
      //Print connection table for all the routers by default
      for(;node_index<totalRouter;node_index++){
    	printConnectionTable(node_index);
      }
      return 1;
}

/*
 *Populates the values of each router in network list and calls shortestPathUpd()
 *if there is a direct link for each router.
 *
 */
void initNetwork()
{
    //Set each routers own values from the originalNetwork matrix
    int routerNum = 0;
    struct router *curRouter;
    for(;routerNum<totalRouter;routerNum++){
     curRouter =&network[routerNum];
     curRouter->routerNum = routerNum;
     int nextRouter=0;
     for(;nextRouter<totalRouter;nextRouter++){
       curRouter->directLink[routerNum][nextRouter] = originalNetwork[routerNum][nextRouter];
     }
   }
    //For each router check if there is direct link,if yes then call shortestPathUpd()
    routerNum = 0;
    for(;routerNum<totalRouter;routerNum++){
		curRouter =&network[routerNum];
		//Send LSP packet to neighbours
		int nextRouterNum=0;
		for(;nextRouterNum<totalRouter;nextRouterNum++)
		{  //Check if a neighbour exists
			if(curRouter->directLink[routerNum][nextRouterNum] > 0){
				int source = curRouter->routerNum;
				int destination = nextRouterNum;
				int * pathArray = curRouter->directLink[source];
				shortestPathUpd(source,source,destination,pathArray,1);
			}
		}
    }
}

/*
 * Updates the source topology in destination router and all direct links in destination router.
 */
void shortestPathUpd(int senderIndex, int source, int destination, int * sourceLinks, int seqNumber)
{
  //get the destindex node
  struct router * dstRouter = &network[destination];
  //To check if this router was updated for the same source
  if(dstRouter->sequenceNum[source] >= seqNumber)
    return;
  //seqenceNum is marked to identify this router was used for this source
  dstRouter->sequenceNum[source] = seqNumber;

  //Update the direct link of destination with topology of source
  int nextRouterNum=0;
  for(;nextRouterNum<totalRouter;nextRouterNum++)
  {
      dstRouter->directLink[source][nextRouterNum] = sourceLinks[nextRouterNum];
  }
  //Update directLink of routers direct with destination router
  nextRouterNum=0;
  for(;nextRouterNum<totalRouter;nextRouterNum++){
    if(dstRouter->directLink[destination][nextRouterNum] > 0)
    {
      if(nextRouterNum != source && nextRouterNum != senderIndex)
	    shortestPathUpd(destination, source,nextRouterNum,sourceLinks,1);
    }
  }
}

/*
 * Print the values of orginal Matrix which was read from the file
 */
void printTopologyMatrix()
{
  int i,j;
  printf("%s%s%s","\x1b[34;1m","\n\n\t Review original topology matrix: \n\t","\x1b[39;49m");
  for (i = 0; i < totalRouter; ++i){
    for (j = 0; j < totalRouter; ++j){
    	if(originalNetwork[i][j] == 999999)
           printf ("%s%d%s\t","\x1b[34;1m",-1,"\x1b[39;49m");
    	else
    		printf ("%s%d%s\t","\x1b[34;1m",originalNetwork[i][j],"\x1b[39;49m");
    }
     printf("%s%s%s","\x1b[34;1m",("\n\t"),"\x1b[39;49m");
  }

}

/*
 * Find the shortest path from the given router to all the routers and
 * update path of the router and print the connection table
 */
void printConnectionTable(int routerNum)
{
   struct router * curRouter = &network[routerNum];
   //Set values into the shortestPath structure of current Router
   initShortestPath(&curRouter->path[routerNum],0,-1,0);
   //set values in path of all the routers directly connected to the router
   int nextRouter =0;
   for(;nextRouter<totalRouter;nextRouter++){
     int cost = curRouter->directLink[routerNum][nextRouter];
     //Check if there is a direct link
     if(cost > 0 && nextRouter != routerNum){
       initShortestPath(&curRouter->path[nextRouter],cost,routerNum,0);
     }
     else if(cost < 0 && nextRouter != routerNum){
       //if there is not direct link cost is set to 999999 and previous router is set to -1
       initShortestPath(&curRouter->path[nextRouter],999999,-1,0);
     }
   }
   //Find the shortest path using Dijkstra's algorithm
   int counter = 0;
   while(counter < totalRouter+1){
		 int minimumCost = 999999;
		 int routerMin= -1;
		 int i = 0;
		 //find router with least cost
		 for(; i < totalRouter; i++){
		   int totalCost = curRouter->path[i].totalCost;
		   if(!curRouter->path[i].costFound && totalCost<=minimumCost){
			 minimumCost = totalCost;
			 routerMin = i;
		   }
		 }
		 if(routerMin != -1)
		 {
		   //minimum cost router is added to the path
		  curRouter->path[routerMin].costFound = 1;
		  //routers directly connected are also update with the minimum cost router
		  int  * nextDirectLink = curRouter->directLink[routerMin];
		  int i = 0;
		  for(; i < totalRouter; i++){
			int pathCost = *(nextDirectLink + i);
			if(pathCost > 0 && !curRouter->path[i].costFound){
			  //Check id the total cost is less than the old cost,if yes update router and cost
			  int oldCost = curRouter->path[i].totalCost;
			  int newCost = curRouter->path[routerMin].totalCost + pathCost;
			  if(newCost < oldCost){
				curRouter->path[i].previousRouter = routerMin;
				curRouter->path[i].totalCost = newCost;
			  }
			}
		  }
		 }
		  counter++;
   }
  //Printing the connection table
   printf ("\n\n\t%s Router %d Connection Table  \n\t Destination\t\t Interface \n\t =================================== %s\n","\x1b[34;1m",routerNum+1,"\x1b[39;49m");
   int j=0;
   for(;j<totalRouter;j++){
     if(curRouter->path[j].previousRouter == -1 || curRouter->path[j].previousRouter == routerNum){
    	 if(j == routerNum){
    		 printf ("%s\t\t%d\t\t --%s\n","\x1b[34;1m", j+1,"\x1b[39;49m");
    	 }else{
    		 if(curRouter->path[j].totalCost >= 999999){
    			 printf ("%s\t\t%d\t\t --%s\n","\x1b[34;1m", j+1,"\x1b[39;49m");
    		 }else{
    			 printf ("%s\t\t%d\t\t %d%s\n","\x1b[34;1m", j+1, j+1,"\x1b[39;49m");
    		 }
    	 }
     }else{
       //Finding router next to the source which is in the shortest path to the destination
       int nextRouter = -1, cnt=0;
       nextRouter = curRouter->path[j].previousRouter;
       while(curRouter->path[nextRouter].previousRouter != routerNum && cnt < totalRouter){
      	 nextRouter = curRouter->path[nextRouter].previousRouter;
      	 cnt++;
       }
       printf ("%s\t\t%d\t\t %d%s\n","\x1b[34;1m", j+1, nextRouter+1,"\x1b[39;49m");
     }
   }
}


/*
 * This function is used to get the input the user enter in console
 * so that they can be printed in red
 *
 */
int getInput() {
	//Reference :http://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/getpass.html
    struct termios oldtc, newtc;
    int ch;
    tcgetattr(STDIN_FILENO, &oldtc);
    newtc = oldtc;
    newtc.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newtc);
    ch=getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
    return ch;
}

/*
 * This function is setter for the shortestPath structure
 */

void initShortestPath(struct shortestPath * path, int totalCost, int previousRouter,int costFound)
{
  //pnode->index = index;
  path->totalCost = totalCost;
  path->previousRouter = previousRouter;
  path->costFound = costFound;
}


/*
 * This function prints the path from source to destination
 */
void printPath(struct router * srcRouter,  int pnode_index, int pathlength)
{
  int prev_index = srcRouter->path[pnode_index].previousRouter;
   if(prev_index != srcRouter->routerNum && pathlength > 0){
    //Call recursively till you find next router in the path
    printPath(srcRouter,  prev_index,  pathlength -1);
  }
  //when found print the router
  if(pathlength != totalRouter - 1)
    printf ("%s%d - %s","\x1b[34;1m",prev_index + 1, "\x1b[39;49m");
  else
	  printf ("%s%d%s ","\x1b[34;1m",prev_index + 1, "\x1b[39;49m");
}

/*
 * This function prints the path from source to destination and the total cost
 * from source to destination
 */
void PrintShortestPath(int source, int destination)
{
  struct router * srcRouter = &network[source];
  //check if the path is generated else generate it
  if(!(srcRouter->path[source].previousRouter == -1 && srcRouter->path[source].costFound == 1))
    printConnectionTable(source);
  int cost = srcRouter->path[destination].totalCost;
  //check if the source or router is deleted
  if(cost>= 99999){
	  int srcValue = originalNetwork[source][source];
	  int destValue = originalNetwork[destination][destination];
	  if(srcValue == 999999 && destValue == 999999){
		  printf ("\n\n\t%sSource and destination routers are down.Please enter another source and destination routers.%s","\x1b[34;1m", "\x1b[39;49m");
	  }else{
		  if(srcValue == 999999)
			printf ("\n\n\t%sSource router is down.Please enter another source router.%s","\x1b[34;1m", "\x1b[39;49m");
		  if(destValue == 999999)
			  printf ("\n\n\t%sDestination router is down.Please enter another destination router.%s","\x1b[34;1m", "\x1b[39;49m");
	  }
  }else{
  printf ("\n\n\t%sThe shortest path from %d to %d is :%s","\x1b[34;1m", source+1, destination+1, "\x1b[39;49m");
  //To print the routers in the path from source to destination
  printPath(srcRouter, destination, totalRouter - 1);
  //To print the cost
  printf ("%s - %d \t,the total cost (value) :%d %s","\x1b[34;1m",destination+1,srcRouter->path[destination].totalCost, "\x1b[39;49m");
  }

}

/*
 * This function update the originalTopology with the deleted router
 * and initializes all the router and shortest path.If source and
 * destination are selected then it prints the new shortest path else
 * displays connection table of all the routers in the network
 */
void updateTopology(int source, int destination,int deletedRouter)
{
	int routerNum=0;
	  for(;routerNum<totalRouter;routerNum++)
	  {   //setting the cost of deleted router to infinity(999999)
		  originalNetwork[deletedRouter][routerNum] = 999999;
		  originalNetwork[routerNum][deletedRouter] = 999999;
	  }
	  int i;
	  //initializing the network list
	  for(i = 0;i< ROUTER_MAX;i++){
		  network[i] = EmptyRouter;
	  }
	  initNetwork();
	  //print the new topology
	  printTopologyMatrix();
	  //Handling if source and destination are not selected
	  if(source == -2 || destination == -2){
		  routerNum=0;
		  //print conection table of all the routers
		  for(;routerNum<totalRouter;routerNum++){
			  printConnectionTable(routerNum);
		  }

	  }else{
		  //print the new shortest path of source and destination
		  PrintShortestPath(source , destination);
	  }

}

/*
 * This function displays the user input in console in red
 */
void displayRedInput(char * userInput, int size )
{
  int i =0;
  while((userInput[i] = getInput())!='\n' && i < size-1){
	printf("%s%c%s","\x1b[31;1m",userInput[i],"\x1b[39;49m");
    i++;
  }
  userInput[i]='\0';
}

/*
 * This is the main function.It take the menu input and displays result accordingly.
 */
int main()
{
  char menuSelected ;
  char source[3],destination[3],down[3],inputArray[2];
  while(menuSelected!='5')
  {  //get menu index from the user
     printf("%s%s%s","\x1b[34;1m","\n\n\n\t (1) Load a Network Topology \n\t (2) Display Connection Table  \n\t (3) Shortest Path to Destination Router \n\t (4) Modify the Network Topology  \n\t (5) Exit \n\n\t Command:","\x1b[39;49m");
     displayRedInput(inputArray,sizeof(inputArray));
     menuSelected = inputArray[0];
     switch(menuSelected)
      {
		case '1' :
		  //get the file name from the user
		  printf("%s%s%s","\x1b[34;1m","\n\t  Input original network topology matrix data file: ","\x1b[39;49m");
		  displayRedInput(topologyFileName,sizeof(topologyFileName));
          //Handling for taking only txt file as input
		  char * end = strrchr(topologyFileName, '.');
		  if(end != NULL){
			  if(strcmp(end, ".txt") == 0){

				  if(getOriginalTopology() < 0){
					printf("\n \t File %s could not be loaded. Please check location and try again\n", topologyFileName);
				  }
			  }else{
				  printf("\n \t Please check and load .txt file.\n");
			  }
		  }else{
			  printf("\n \t File %s could not be loaded. Please check location and try again\n", topologyFileName);
		  }
		  break;
		case '2' :
			//get the source router from the user
		  printf("%s%s%s","\x1b[34;1m","\n\t Please select a router : ","\x1b[39;49m");
		  displayRedInput(source,sizeof(source));
		  sourceRouter = atoi(source);
		  if(sourceRouter >= 1 && sourceRouter <= totalRouter){
			  printConnectionTable(sourceRouter-1);
		  }else{
			printf("%s%s%s","\x1b[34;1m","\n\tWrong router index entered. Try again : ","\x1b[39;49m");
		  }
		  break;
		case '3' :
			//get the destination from the user
		  printf("%s%s%s","\x1b[34;1m","\n\t Please select a destination : ","\x1b[39;49m");
		  displayRedInput(destination,sizeof(destination));
		  int srcrouterNumber = sourceRouter;
		  int dstrouterNumber = atoi(destination);
		  destinationRouter = dstrouterNumber;
		  if(srcrouterNumber >= 1 && srcrouterNumber <= totalRouter && dstrouterNumber >= 1 && dstrouterNumber <= totalRouter){
			PrintShortestPath(srcrouterNumber-1 , dstrouterNumber-1);
		  }else{
			printf("%s%s%s","\x1b[34;1m","\tWrong destination key entered. Try again : ","\x1b[39;49m");
		  }
		  break;
		case '4' :
			 //get the router to be deleted from the user
			  printf("%s%s%s","\x1b[34;1m","\n\t  Select a router to be removed: ","\x1b[39;49m");
			  displayRedInput(down,sizeof(down));
			  int deletedRouter = atoi(down);
			  updateTopology(sourceRouter-1 , destinationRouter-1,deletedRouter-1);

			  break;
		case '5' :
			//exit the program
			printf("%s%s%s","\x1b[34;1m","\n\t Exit CS542 project.  Good Bye!","\x1b[39;49m");
		  return(0);

		default :  printf("%s%s%s","\x1b[34;1m","\tWrong menu key entered. Try again : ","\x1b[39;49m");
      }

    fflush(stdin);

  }
}



