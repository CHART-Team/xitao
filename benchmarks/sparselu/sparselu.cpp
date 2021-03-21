#include "sparselu_taos.h"  
#include "sparselu.h"
//#define TEST_SERIAL
vector<string> get_tao_name(AssemblyTask* task) 
{
 vector<string> meta_data;
#ifdef OUTPUT_DOT
  stringstream st; 
  stringstream st_address; 
  string address;
  string raw_name;
  string color;
  BDIV* bdiv = dynamic_cast<BDIV*>(task);
  BMOD* bmod = dynamic_cast<BMOD*>(task);
  LU0* lu0   = dynamic_cast<LU0*>(task);
  FWD* fwd   = dynamic_cast<FWD*>(task);
  const int digits = 3;
  if(bdiv) {
    st_address << bdiv;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "bdiv_"<< address;
    raw_name = "bdiv";
    color = "tomato";
  } else if (bmod){
    st_address << bmod;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "bmod_"<< address;
    raw_name = "bmod"; 
    color = "darkolivegreen";
  } else if (lu0) {
    st_address << lu0;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "lu0_"<< address;
    raw_name = "lu0";
    color = "darkseagreen";
  } else if (fwd) {
    st_address << fwd;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "fwd_"<< address;
    raw_name = "fwd";
    color = "cornflowerblue";
  } else {
    std::cout << "error: unknown TAO type" << std::endl;
    exit(0);
  }  
  meta_data.push_back(st.str());
  meta_data.push_back(raw_name);
  meta_data.push_back(color);
#endif
  return meta_data; 
}

void sparselu_parallel() 
{
   float t_start,t_end;
   float time;
   int ii, jj, kk;
   int bcount = 0;
   ofstream file;
   init_dot_file(file, "sparselu.dot");

   generate_matrix_structure(bcount);

#ifdef NO_STA
   generate_matrix_values();
#endif  
   //printf("Init OK Matrix is: %d (%d %d) # of blocks: %d memory is %ld MB\n", (NB*BSIZE), NB, BSIZE, bcount, BSIZE*BSIZE*bcount*sizeof(ELEM)/1024/1024);
   // init XiTAO runtime 

   xitao_init();
#ifndef NO_STA
   NTHREADS = xitao_nthreads;
   STA_IND = 0;
#endif
   //std::cout << "init " << std::endl;
   int prev_diag = -1;
   //Timing Start
   t_start=get_time();
   for (kk=0; kk<NB; kk++) {

      auto lu0 = new LU0(A[kk][kk], BSIZE, TAO_WIDTH); 
#ifndef NO_STA
      lu0->set_sta(get_sta_val(kk,kk));
#endif
#if STA_AWARE_STEALING
      lu0->workload_hint = get_sta_int_val(kk, kk); 
#endif

      //cout << "Creating LU0: " << kk << " " << kk << endl;
      //lu0_objs.push_back(new LU0(A[kk][kk], BSIZE, TAO_WIDTH));

      //check if the task will get its input this from a previous task
      if(dependency_matrix[kk][kk]) {
        write_edge(file, get_tao_name(dependency_matrix[kk][kk]), kk, kk, get_tao_name(lu0), kk, kk);
        dependency_matrix[kk][kk]->make_edge(lu0);
      } else {
#ifndef NO_STA
         auto init0 = new INITB(&A[kk][kk], true, BSIZE, TAO_WIDTH);
         init0->set_sta(get_sta_val(kk,kk));
#if STA_AWARE_STEALING
         init0->workload_hint = get_sta_int_val(kk, kk); 
#endif
         init0->make_edge(lu0);
         xitao_push(init0);
         init_matrix[kk][kk] = true;
#else        
         xitao_push(lu0);
#endif
      } 
      dependency_matrix[kk][kk] = lu0;

      for (jj=kk+1; jj<NB; jj++)
         if (A[kk][jj] != NULL) {
            //fwd(A[kk][kk], A[kk][jj]);
           auto fwd =  new FWD(A[kk][kk], A[kk][jj], BSIZE, TAO_WIDTH);
#ifndef NO_STA
            fwd->set_sta(get_sta_val(kk,kk));
#endif
#if STA_AWARE_STEALING
            fwd->workload_hint = get_sta_int_val(kk, kk); 
#endif
            if(dependency_matrix[kk][kk]) {
              write_edge(file, get_tao_name(dependency_matrix[kk][kk]), kk, kk, get_tao_name(fwd), kk, jj);
#ifndef NO_STA
              if(!init_matrix[kk][jj]) {
                auto init0 = new INITB(&A[kk][jj], true, BSIZE, TAO_WIDTH); 
                init0->set_sta(get_sta_val(kk,jj));
#if STA_AWARE_STEALING
                init0->workload_hint = get_sta_int_val(kk, jj); 
#endif
                init0->make_edge(fwd);
                xitao_push(init0);
                init_matrix[kk][jj] = true;
              }
#endif
              dependency_matrix[kk][kk]->make_edge(fwd);
            }

            if(dependency_matrix[kk][jj]) {
              write_edge(file, get_tao_name(dependency_matrix[kk][jj]), kk, jj, get_tao_name(fwd), kk, jj);
              dependency_matrix[kk][jj]->make_edge(fwd);
            }
           
           dependency_matrix[kk][jj] = fwd;
         }

      for (ii=kk+1; ii<NB; ii++) 
        if (A[ii][kk] != NULL) {
        // if(is_null_entry(ii, kk) == FALSE) {
            //bdiv (A[kk][kk], A[ii][kk]);
            auto bdiv = new BDIV(A[kk][kk], A[ii][kk], BSIZE, TAO_WIDTH);
#ifndef NO_STA
            bdiv->set_sta(get_sta_val(ii,kk));
#endif
#if STA_AWARE_STEALING
            bdiv->workload_hint = get_sta_int_val(ii, kk); 
#endif

            if(dependency_matrix[kk][kk]) {
              write_edge(file, get_tao_name(dependency_matrix[kk][kk]), kk, kk, get_tao_name(bdiv), ii, kk);
              dependency_matrix[kk][kk]->make_edge(bdiv);
            }

            if(dependency_matrix[ii][kk]) {
              write_edge(file, get_tao_name(dependency_matrix[ii][kk]), ii, kk, get_tao_name(bdiv), ii, kk);
              dependency_matrix[ii][kk]->make_edge(bdiv);
            }
#ifndef NO_STA
            if(!init_matrix[ii][kk]) { 
              auto init0 = new INITB(&A[ii][kk], true, BSIZE, TAO_WIDTH); 
              init0->set_sta(get_sta_val(ii,kk));
#if STA_AWARE_STEALING
              init0->workload_hint = get_sta_int_val(ii, kk); 
#endif
              init0->make_edge(bdiv);
              xitao_push(init0);
              init_matrix[ii][kk] = true;
            }
#endif
            dependency_matrix[ii][kk] = bdiv;
         }

      for (ii=kk+1; ii<NB; ii++) {
         if (A[ii][kk] != NULL) {
         //if(is_null_entry(ii, kk) == FALSE) {
            for (jj=kk+1; jj<NB; jj++) {
               if (A[kk][jj] != NULL) {
#ifdef NO_STA                  
                  if (A[ii][jj]==NULL) {
                    A[ii][jj]=allocate_clean_block();
                  }
#endif
                  //bmod(A[ii][kk], A[kk][jj], A[ii][jj]);
                  auto bmod = new BMOD(A[ii][kk], A[kk][jj], &A[ii][jj], BSIZE, TAO_WIDTH);
#ifndef NO_STA
                  bmod->set_sta(get_sta_val(ii,kk));
#endif
#if STA_AWARE_STEALING
                  bmod->workload_hint = get_sta_int_val(ii, kk); 
#endif

#ifndef NO_STA
                  if(!init_matrix[ii][kk]) {
                    auto init0 = new INITB(&A[ii][kk], true, BSIZE, TAO_WIDTH);
                    init0->set_sta(get_sta_val(ii, kk));
#if STA_AWARE_STEALING
                    init0->workload_hint = get_sta_int_val(ii, kk); 
#endif
                    init0->make_edge(bmod);
                    xitao_push(init0);
                    init_matrix[ii][kk] = true;

                  }
                  if(!init_matrix[kk][jj]) {
                    auto init0 = new INITB(&A[kk][jj], true, BSIZE, TAO_WIDTH);
                    init0->set_sta(get_sta_val(kk,jj));
#if STA_AWARE_STEALING
                    init0->workload_hint = get_sta_int_val(kk, jj); 
#endif

                    init0->make_edge(bmod);
                    xitao_push(init0);
                    init_matrix[kk][jj] = true;
                  }
                  if(!init_matrix[ii][jj]) {
                    A[ii][jj] = new ELEM[BSIZE * BSIZE];
                    auto init0 = new INITB(&A[ii][jj], false, BSIZE, TAO_WIDTH);
                    init0->set_sta(get_sta_val(ii,jj));
#if STA_AWARE_STEALING
                    init0->workload_hint = get_sta_int_val(ii, jj); 
#endif

                    init0->make_edge(bmod);
                    xitao_push(init0);
                    init_matrix[ii][jj] = true;
                  }
#endif

                  if(dependency_matrix[ii][kk]) {
                      write_edge(file, get_tao_name(dependency_matrix[ii][kk]), ii, kk, get_tao_name(bmod), ii, jj);
                      dependency_matrix[ii][kk]->make_edge(bmod);
                  }

                  if(dependency_matrix[kk][jj]) {
                      write_edge(file, get_tao_name(dependency_matrix[kk][jj]), kk, jj, get_tao_name(bmod), ii, jj);
                      dependency_matrix[kk][jj]->make_edge(bmod);
                  }

                  if(dependency_matrix[ii][jj]) {
                    write_edge(file, get_tao_name(dependency_matrix[ii][jj]), ii, jj, get_tao_name(bmod), ii, jj);
                    dependency_matrix[ii][jj]->make_edge(bmod);
                  }

                  dependency_matrix[ii][jj] = bmod;
               }
            }
         }
      }
   }
   //xitao_init();
   //Timing Stop
   
   t_end=get_time();
   double dagtime = t_end-t_start;
   
   close_dot_file(file);
   //Timing Start
   t_start=get_time();

   //xitao_push(lu0_objs[0]);
   //Start the TAODAG exeuction
   xitao_start();
   //Finalize and claim resources back
   xitao_fini();

   //Timing Stop
   t_end=get_time();
   //gotao_wait();
   time = t_end-t_start;
  // printf("Matrix is: %d (%d %d) memory is %ld MB time to compute in parallel: %11.4f sec\n",
   //(NB*BSIZE), NB, BSIZE, (NB*BSIZE)*(NB*BSIZE)*sizeof(ELEM)/1024/1024, time);
  // printf("%s\n", std::string(104, '=').c_str());
  // printf("Runtime Stats\n");
  // printf("%s\n", std::string(104, '=').c_str());
  // printf("%-12s %-12s %-12s %-12s %-12s %-12s %-12s %-12s\n","NB", "BSIZE", "Size [MB]", "Total Tasks", "Throughput", "Steals" , "DAG Building", "Par Exec Time"); 
   printf("%-12d %-12d %-12ld %-12lu %-12.4f %-12.4f\n", NB, BSIZE, (NB*BSIZE)*(NB*BSIZE)*sizeof(ELEM)/1024/1024, tao_total_steals, dagtime, time); 
   fflush(stdout);
   //printf("Total Tasks: %lu \nThroughput: %11.4f Tasks/sec\nTotal Steals: %lu\n", PolyTask::total_taos, PolyTask::total_taos/time, tao_total_steals);
   // print_structure();
}

void sparselu_serial() 
{
  float t_start,t_end;
  float time;
  int ii, jj, kk;
  int bcount = 0;
  generate_matrix_structure(bcount);
  generate_matrix_values();
  //print_structure();
  printf("Init OK Matrix is: %d (%d %d) # of blocks: %d memory is %ld MB\n", (NB*BSIZE), NB, BSIZE, bcount, bcount*sizeof(ELEM)/1024/1024);

  //Timing Start
  t_start=get_time();

  for (kk=0; kk<NB; kk++) {

     lu0(A[kk][kk], BSIZE);

     for (jj=kk+1; jj<NB; jj++)
        if (A[kk][jj] != NULL)
           fwd(A[kk][kk], A[kk][jj], BSIZE);

     for (ii=kk+1; ii<NB; ii++) 
        if (A[ii][kk] != NULL)
           bdiv(A[kk][kk], A[ii][kk], BSIZE);

     for (ii=kk+1; ii<NB; ii++) {
        if (A[ii][kk] != NULL) {
           for (jj=kk+1; jj<NB; jj++) {
              if (A[kk][jj] != NULL) {
                 if (A[ii][jj]==NULL)
                 {
                    A[ii][jj]=allocate_clean_block();
                 }

                 bmod(A[ii][kk], A[kk][jj], A[ii][jj], BSIZE);
              }
           }
        }
     }
  }

  //Timing Stop
  t_end=get_time();

  time = t_end-t_start;
  printf("Matrix is: %d (%d %d) memory is %ld MB time to compute in serial: %11.4f sec\n", 
        (NB*BSIZE), NB, BSIZE, (NB*BSIZE)*(NB*BSIZE)*sizeof(ELEM)/1024/1024, time);

}

/* ************************************************************ */
/* main               */
/* ************************************************************ */

int main(int argc, char** argv) {
  if(argc < 4) {
    printf("Usage: ./sparselu BLOCKS BLOCKSIZE RESOURCE_HINT <ITER USE_DENSE>\n");
    exit(0);
  }
  NB = atoi(argv[1]);
  BSIZE = atoi(argv[2]);
  TAO_WIDTH = atoi(argv[3]);
  int rep = (argc > 4)? atoi(argv[4]) : 1;
  USE_DENSE = (argc > 5)? atoi(argv[5]) : 0;
  printf("%s\n", std::string(130, '=').c_str());
  printf("Runtime Stats\n");
  printf("%s\n", std::string(130, '=').c_str());
  printf("%-12s %-12s %-12s %-12s %-12s %-12s\n","NB", "BSIZE", "Size [MB]", "Steals" , "DAG Building", "Par Exec Time"); 
  while(rep-- > 0) {
    A.resize(NB);
    dependency_matrix.resize(NB);
#ifndef NO_STA    
    init_matrix.resize(NB);
    sta.resize(NB);
    for(int i = 0; i < NB; ++i){
      init_matrix[i].resize(NB);
      init_matrix[i].assign(NB, false);
      sta[i].resize(NB); 
      sta[i].assign(NB, -1.0f);
    }
#endif
    for(int i = 0; i < NB; ++i) {
      A[i].resize(NB);
      A[i].assign(NB, NULL);
      dependency_matrix[i].resize(NB);
      dependency_matrix[i].assign(NB, NULL);
    }
    sparselu_parallel();
    if(rep > 0) {
      for(int i = 0; i < NB; ++i) { 
        for(int j = 0; j < NB; ++j) { 
          if(A[i][j] != NULL) delete[] A[i][j];
        } 
      }
    }
  }
#ifdef TEST_SERIAL  
  auto res_parallel = read_structure();
  sparselu_serial();
  auto res_serial = read_structure();
  ELEM diff = 0.0;
  ELEM norm = 0.0;
  if(res_parallel.size() != res_serial.size()) {
    printf("Result vectors are not equally sized\n");
  }
  else {
    for(int i = 0; i < res_parallel.size(); i++){
      if(std::isnan(res_serial[i]) || std::isnan(res_parallel[i])) continue;
      if(!std::isfinite(res_serial[i]) || !std::isfinite(res_parallel[i])) continue;
      diff += (res_serial[i] - res_parallel[i]) * (res_serial[i] - res_parallel[i]);
      norm += res_serial[i] * res_serial[i];
    }
    if(std::isnan(sqrt(diff/norm)) || !std::isfinite(sqrt(diff/norm))) printf("Error is too high\n");
    else printf("%-20s : %8.5e\n","Rel. L2 Error", sqrt(diff/norm));
  }
#endif
}

