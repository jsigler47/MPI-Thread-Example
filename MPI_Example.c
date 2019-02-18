#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv)
{
	int debug = 0; //Set to 1 for debug output.
	int N, H, sum, max, min;
	int dataList[25]; //Where workers will store their own partial lists.
	int BigList[500] = { 0 }; //Max of 20 workers and 25 data points each, 20*25=500;

	for(int i = 0; i < 500; i++)
		BigList[i] = -1; //Initialize entire array to -1 to know if it's been utilized later on. (Saves from printing the whole thing if we don't use max N and max workers)

	if(argc != 3)
	{
		printf("N and H arguments required\n");
		exit(1);
	}

	MPI_Init(&argc, &argv);

	int my_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	
	if(my_rank == 0)
	{	//0 is the only one that accepts N and H from the command line args.
		N = atoi(argv[1]);
		H = atoi(argv[2]);
		printf("N: %d H: %d\n", N, H);

		//0 broadcasts N and H to all other nodes.
		MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&H, 1, MPI_INT, 0, MPI_COMM_WORLD);

		printf("Generating data lists...\n");

		if(debug == 1)
			printf("0 Sent N and H...Creating dataList\n");

		for(int i = 0; i < N; i++)
			dataList[i] = i % H + 1;
	}
	else
	{	//Everyone receives N and H from 0.
		MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&H, 1, MPI_INT, 0, MPI_COMM_WORLD);

		if(debug == 1)
			printf("%d Received N: %d and H: %d ...Creating dataList\n", my_rank, N, H);

		for(int i = 0; i < N; i++)
			dataList[i] = i % H + 1;

	}
	if(debug == 1)
	{
		for(int i = 0; i < N; i++)
			printf("%d : datalist[%d] = %d\n", my_rank, i, dataList[i]);
	}

	MPI_Barrier(MPI_COMM_WORLD); //Wait for everyone to compile their datalist
	if(my_rank == 0)
	{
		printf("Data lists generated.\n");
		printf("Gathering datalists...\n");
	}

	MPI_Gather(dataList, N, MPI_INT, &BigList, N, MPI_INT, 0, MPI_COMM_WORLD); //Everyone sends their datalist to node 0s biglist.

	if(my_rank == 0)
	{
		printf("Data lists gathered.\n");
		if(debug == 1)
		{	//Print the big list gathered by node 0.
			int tmp = 0;
			printf("0 gathered dataLists into BigList. \nBigList:\n");
			for(int i = 0; i < 500; i++)
			{
				if(BigList[i] != -1)
				{
					printf("%3d : %3d\n", i, BigList[i]);
					tmp += BigList[i];
				}
			}
			printf("Sum of bigList before sending: %d\n", tmp);
		}

	}
	//Boss shares BigList with everyone.
	MPI_Bcast(&BigList, 500, MPI_INT, 0, MPI_COMM_WORLD);
	if(my_rank == 0)
		printf("Calculating partial mins, maxes, and sums\n");
	max = sum = 0;
	min = H + 1; //Higher than the highest possible data value, H.

	for(int i = my_rank * N; i < my_rank * N + N; i++) //Start at nodes spot in biglist and end before next nodes spot.
	{//Every node calculates their own min, max, and sum for their set of biglist.
		if(BigList[i] < min)
			min = BigList[i];

		if(BigList[i] > max)
			max = BigList[i];

		sum += BigList[i];
	}
	MPI_Barrier(MPI_COMM_WORLD); //Wait for all data to be calculated before gathering.
	//Every node sends 0 it's min, max and sum for it's partial list.
	MPI_Send(&min, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(&sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

	if(my_rank == 0)
	{
		int tmpMin, tmpMax, tmpSum;

		for(int i = 1; i < world_size; i++)
		{//Receive each nodes min, max, and sum and compare/add with 0s min, max, and sum.
			MPI_Recv(&tmpMin, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
			MPI_Recv(&tmpMax, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
			MPI_Recv(&tmpSum, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);

			if(tmpMin < min)
				min = tmpMin;
			if(tmpMax > max)
				max = tmpMax;
			sum += tmpSum;
		}
		//0 Reports global min, max, and sum based on partials from other nodes and self.
		printf("Global Min: %d\nGlobal Max: %d\nGlobal Sum: %d\n", min, max, sum);
	}

	MPI_Finalize();

	return 0;
}