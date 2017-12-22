#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

#define tolerance 0.001
#define F(x)  exp(-x*x)
typedef struct data_stack stack_node;
typedef struct top_stack stack;

struct data_stack {
	double data[2];
	stack_node *next;
};

struct top_stack {
	stack_node *top;
};

stack * new_stack() {
	stack *n;
	n = (stack *) malloc (sizeof(stack));
	n->top = NULL;
	return n;
}

void free_stack(stack *s) {
	free(s);
}

void push (double *data, stack *s) {
	stack_node *n;
	n = (stack_node *) malloc (sizeof(stack_node));
	n->data[0] = data[0];
	n->data[1] = data[1];
	if (s->top == NULL) {
		n->next = NULL;
		s->top  = n;
	}
	else {
		n->next = s->top;
		s->top = n;
	}
}

double * pop (stack * s) {
	stack_node * n;
	double *data;
	if (s == NULL || s->top == NULL) {
		return NULL;
	}
	n = s->top;
	s->top = s->top->next;
	data = (double *) malloc(2*(sizeof(double)));
	data[0] = n->data[0];
	data[1] = n->data[1];
	free (n);
	return data;
}

int is_empty (stack * s) {
  return (s == NULL || s->top == NULL);
}

double master(int number_of_processes,int a,int b) {
	MPI_Status status;
	double data_buffer[] = {0,0};
	int slaves = number_of_processes - 1;
	int idle = 0;
	int* slave_list = (int*) malloc(sizeof(int)*(slaves));
	double result = 0;


	stack* s;
	s = new_stack();
	data_buffer[0] = a;
	data_buffer[1] = b;
	push(data_buffer, s);

	int i;
	for (i=0;i<slaves;i++){
		slave_list[i] = 0;
	}
	do {
		MPI_Recv(data_buffer, 2, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		idle++;
		slave_list[status.MPI_SOURCE - 1] = 1;
		if (status.MPI_TAG == 1) {
			result += data_buffer[0];
		}
		else {
			push(data_buffer,s);
      			MPI_Recv(data_buffer, 2, MPI_DOUBLE, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
      			push(data_buffer,s);
    		}
		int j = 0;
		while(!is_empty(s) && idle>0) {
			if (slave_list[j]) {
				MPI_Send(pop(s), 2, MPI_DOUBLE, j+1, 0, MPI_COMM_WORLD);
				slave_list[j] = 0;
				idle--;
      			}
      		j = (j+1) % slaves;
    		}
	} while(!is_empty(s) || idle!=slaves);
	for (i=0;i<slaves;i++) {
		data_buffer[0] = 0;
		data_buffer[1] = 0;
		MPI_Send(data_buffer, 2, MPI_DOUBLE, i+1, 1, MPI_COMM_WORLD);
	}
	return result;
}

void slave(int process_rank) {
	MPI_Status status;
	double data_buffer[] = {0,0};
	MPI_Send(data_buffer, 2, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
	while (1) {
		MPI_Recv(data_buffer, 2, MPI_DOUBLE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG != 1) {
	    		double left = data_buffer[0];
	    		double right = data_buffer[1];
	    double lrarea = (F(left) + F(right) + 4*F((left + right)/2)) * (right - left) / 6;
			double mid, fmid, larea, rarea;
			mid = (left + right) / 2;
			fmid = F(mid);
			larea = (F(left) + fmid + 4*F((mid + left)/2)) * (mid - left) / 6;
			rarea = (fmid + F(right) + 4*F((mid + right)/2)) * (right - mid) / 6;
			if (fabs((larea + rarea) - lrarea) > tolerance) {
				data_buffer[0] = left;
				data_buffer[1] = mid;
				MPI_Send(data_buffer, 2, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
				data_buffer[0] = mid;
				data_buffer[1] = right;
				MPI_Send(data_buffer, 2, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
			} else {
				data_buffer[0] = larea + rarea;
				data_buffer[1] = 0;
				MPI_Send(data_buffer, 2, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
	    		}
		}
		else {
			break;
		}
  	}
}

int main(int argc, char **argv ) {
	int i, process_rank, number_of_processes;
	double area;
  clock_t begin,end;
	int a,b;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&number_of_processes);
	MPI_Comm_rank(MPI_COMM_WORLD,&process_rank);
	if(process_rank==0)
	{

  printf("Enter the limit : ");
  scanf("%d",&a);
  scanf("%d",&b);
  }
  begin = clock();
	if (process_rank == 0) {
		area = master(number_of_processes,a,b);
	}
	else {
		slave(process_rank);
	}

	if(process_rank == 0) {
		printf("Integral is %lf\n", area);
  	}
  end = clock();
	//MPI_Reduce( &finish, &totalTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
	if ( process_rank == 0 ) {
			printf( "Total time spent is %f\n", (double)(end - begin) / CLOCKS_PER_SEC  );
	}
  	MPI_Finalize();
  	return 0;
}
