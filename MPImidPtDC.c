#include <stdio.h>
#include<math.h>
#include <mpi.h>

double f(double x) {
    return exp(-x*x);
}

void Input(int p, int process_rank, double* a_p, double* b_p) {
   int q;
   MPI_Status status;
   if (process_rank == 0) {
      printf("Enter a, b\n");
      scanf("%lf %lf", a_p, b_p);

      for (q = 1; q < p; q++) {
         MPI_Send(a_p, 1, MPI_DOUBLE, q, 0, MPI_COMM_WORLD);
         MPI_Send(b_p, 1, MPI_DOUBLE, q, 0, MPI_COMM_WORLD);
      }
   } else {
      MPI_Recv(a_p, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
      MPI_Recv(b_p, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
   }
}

double area(double  local_a, double  local_b, int local_n, double  h) {
    double local_area=0,x,h2;
    int i;
    h2=h/2;//for midpoint
    for (i = 0; i <= local_n-1; i++) {
        x = local_a + i*h;
        local_area = local_area + f(x+h2)*h;
    }
    return local_area;
}

int main(int argc, char** argv) {
    int process_rank,p,n,source,dest=0,tag=0,local_n;
    double a,b,h,local_a,local_b,local_area=0.0,total,start,finish,local_area_b=0.0,total_b;
    MPI_Status  status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    Input(p, process_rank, &a, &b);
    start=MPI_Wtime();
    if(a>b)
    n=(int)(a-b);
    else
    n=(int)(b-a);
    h = (b-a)/n;
    local_n = n/p;

    local_a = a + process_rank*local_n*h;
    local_b = local_a + local_n*h;
    local_area = area(local_a, local_b, local_n, h);

    if (process_rank == 0) {
        total = local_area;
        for (source = 1; source < p; source++) {
            MPI_Recv(&local_area, 1, MPI_DOUBLE, source, tag,
                MPI_COMM_WORLD, &status);
            total = total + local_area;
        }
    } else {
        MPI_Send(&local_area, 1, MPI_DOUBLE, dest,
            tag, MPI_COMM_WORLD);
    }

    if (process_rank == 0) {
        printf("Integral is %.15f\n",total);
    }
    finish=MPI_Wtime()-start;
    double totalTime;
    MPI_Reduce( &finish, &totalTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
    if ( process_rank == 0 ) {
        printf( "Total time spent is %f\n", totalTime );
    }
    MPI_Finalize();
  //  printf("Parallel Elapsed time: %f seconds\n", finish-start);
    return 0;
}
