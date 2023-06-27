#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include "hw2_output.h"

pthread_mutex_t mut;
int r1, c1, r2, c2, r3, c3, r4, c4;
int ** M1;
int ** M2;
int ** M3;
int ** M4;
int ** J;
int ** L;
int ** R;
int row_of_R = 0;
int * conditions_J;
int * conditions_L;
sem_t * sem_J;
sem_t * sem_L;

void print_J(){
    for(int i=0; i<r1; i++){
        for(int j=0; j<c1; j++){
            std::cout<<J[i][j]<<" "; 
        }
        std::cout<<std::endl; 
        
    }
    std::cout<<std::endl; 
}
void* sum_M1_M2_to_J(void* arg){

    int k = long(arg);
    int i;
    //pthread_mutex_lock (&mut);
    for (i = 0; i < c1; i++) {
      J[k][i] = M1[k][i]+M2[k][i];
      hw2_write_output(0,k+1,i+1,J[k][i]);
      conditions_J[k]+=1;
    }

    if(conditions_J[k]==c1) sem_post(&(sem_J[k]));
   //pthread_mutex_unlock (&mut);

    pthread_exit(0);
}

void print_L(){
    for(int i=0; i<r3; i++){
        for(int j=0; j<c3; j++){
            std::cout<<L[i][j]<<" "; 
        }
        std::cout<<std::endl; 
        
    }
    std::cout<<std::endl; 
}
void* sum_M3_M4_to_L(void* arg){
    //
    int k = long(arg);
    int i;
    //pthread_mutex_lock (&mut);
    for (i = 0; i < c3; i++) {
      L[k][i] = M3[k][i]+M4[k][i];
      hw2_write_output(1,k+1,i+1,L[k][i]);
      ++conditions_L[i];
        if(conditions_L[i] == r3) sem_post(&(sem_L[i]));
    }

    //pthread_mutex_unlock (&mut);

    pthread_exit(0);
}

void print_R(){
    for(int i=0; i<r1; i++){
        for(int j=0; j<c3; j++){
            printf("%d ", R[i][j]);
        }
        printf("\n");
        
    }
}

void* multiply_J_L_to_R(void* arg){
    int i = long(arg);
    //pthread_mutex_lock (&mut);
    
    for (int j = 0; j < c3; j++){
        R[i][j]=0;
        
        for (int k = 0; k < r3; k++){

            sem_wait(&(sem_J[i]));
            sem_wait(&(sem_L[j]));
            

            R[i][j] += J[i][k] * L[k][j];
            
        
            sem_post(&(sem_J[i]));            
            sem_post(&(sem_L[j]));
             
        }
        hw2_write_output(2,i+1,j+1,R[i][j]);
            
    }
      
    //pthread_mutex_unlock (&mut);
    pthread_exit(0);
}


int main(){
    hw2_init_output();
    scanf("%d %d", &r1, &c1);

    M1 = new int*[r1];
    for(int i = 0; i < r1; ++i)
        M1[i] = new int[c1];


    for(int row=0; row<r1; row++){
        for(int col=0; col<c1; col++){
            scanf("%d", &(M1[row][col]));
        }
    }

    scanf("%d %d", &r2, &c2);


    M2 = new int*[r2];
    for(int i = 0; i < r2; ++i)
        M2[i] = new int[c2];

    for(int row=0; row<r2; row++)
    {
        for(int col=0; col<c2; col++){
            scanf("%d", &(M2[row][col]));
        }
    }

    scanf("%d %d", &r3, &c3);


    M3 = new int*[r3];
    for(int i = 0; i < r3; ++i)
        M3[i] = new int[c3];

    for(int row=0; row<r3; row++){
        for(int col=0; col<c3; col++){
            scanf("%d", &(M3[row][col]));
        }
    }

    scanf("%d %d", &r4, &c4);


    M4 = new int*[r4];
    for(int i = 0; i < r4; ++i)
        M4[i] = new int[c4];

    for(int row=0; row<r4; row++){
        for(int col=0; col<c4; col++){
            scanf("%d", &(M4[row][col]));
        }
    }


    J = new int*[r1];
    for(int i = 0; i < r1; ++i)
        J[i] = new int[c1];


    L = new int*[r3];
    for(int i = 0; i < r3; ++i)
        L[i] = new int[c3];


    R = new int*[r1];
    for(int i = 0; i < r1; ++i)
        R[i] = new int[c3];

    conditions_J = new int[r1]();
    conditions_L = new int[c3]();
    sem_J = new sem_t[r1];
    sem_L = new sem_t[c3];

    for(int i=0; i<r1; i++) sem_init(&sem_J[i], 0, 0);
    for(int i=0; i<c3; i++) sem_init(&sem_L[i], 0, 0);


    pthread_t thread_for_J[r1];
    pthread_t thread_for_L[r3];
    pthread_t thread_for_R[r1];


    pthread_mutex_init(&mut, NULL);



    for (int i = 0; i < r1; i++)
  		pthread_create(&thread_for_J[i], NULL, sum_M1_M2_to_J, (void*)(i));
        

    for (int i = 0; i < r3; i++)
  		pthread_create(&thread_for_L[i], NULL, sum_M3_M4_to_L, (void*)(i));

    for (int i = 0; i < r1; i++){
        pthread_create(&thread_for_R[i], NULL, multiply_J_L_to_R, (void*)(i));
    }


  	for (int i = 0; i < r1; i++)
  		pthread_join(thread_for_J[i], NULL);


  	for (int i = 0; i < r3; i++)
  		pthread_join(thread_for_L[i], NULL);
        

  	for (int i = 0; i < r1; i++){
        pthread_join(thread_for_R[i], NULL);
    }
  		

    // clean 
    pthread_mutex_destroy(&mut);

    // TESTS
    /*
    print_J();
    print_L();
    */
    print_R();

    
    

    return 0;
}