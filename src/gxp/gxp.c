#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>
#include <time.h>

#include <std/std.h>
#include <xml/topology.h>
#include <xml/parser.h>
#include <struct/rtable.h>
#include <sim/rtable_stat.h>
#include <iface/iface.h>
#include <comm/impl/comm.h>
#include "impl/gxp.h"
#include "impl/gxp_router.h"

static int
Min(int x, int y){
  return x < y ? x : y;
}

static int
gxp_man_same_cluster(gxp_man_t man, int h0, int h1, int prefix){
  const char * h0name = man -> peer_hostnames[h0];
  const char * h1name = man -> peer_hostnames[h1];
  if(strncmp(h0name, h1name, prefix) == 0)
    return 1;
  else
    return 0;
}

static void
shuffle_array(int* a, int n){
  int i, j;
  int x;

  for(i = 0; i < n; i++){
    j = rand() % n;
    x = a[i];a[i] = a[j];a[j] = x;
  }
}

static void
shuffle_and_sort_pairs(float* k, int* v, int n){
  int i, j;
  float tmpk;
  int tmpv;

  /* first shuffle */
  for(i = 0; i < n; i++){
    j = rand() % n;
    tmpk = k[i];k[i] = k[j];k[j] = tmpk;
    tmpv = v[i];v[i] = v[j];v[j] = tmpv;
  }

  /* bubble sort */
  for(i = 0; i < n; i++){
    for(j = 0; j < n - 1; j++){
      if(k[j] > k[j + 1]){
	tmpk = k[j];k[j] = k[j+1];k[j+1] = tmpk;
	tmpv = v[j];v[j] = v[j+1];v[j+1] = tmpv;
      }
    }
  }
}

static double
timeval_diff(struct timeval *tv0, struct timeval *tv1){
  return (tv1->tv_sec - tv0->tv_sec) + (tv1->tv_usec - tv0->tv_usec) * 1e-6;
}

static void
wait_messages(dlfree_comm_node_t comm, int count){
  int src_id;
  void* buff;
  int buffsize;

  for(; count > 0; --count){
    dlfree_comm_node_recv_any_data(comm, &src_id, &buff, &buffsize);
    std_free(buff);
  }
}

/**
   The GXP interface is used to perform collective operations that are made available via the
   GXP mw command.
*/
gxp_man_t
gxp_man_create(){
  char *buf;
  gxp_man_t man;

  man = std_malloc(sizeof(gxp_man));

  buf = std_getenv("GXP_EXEC_IDX");
  man -> gxp_idx = atoi(buf);

  buf = std_getenv("GXP_NUM_EXECS");
  man -> gxp_num_execs = atoi(buf);

  buf = std_getenv("GXP_HOSTNAME");
  man -> gxp_hostname = (char*) std_malloc(sizeof(char) * (strlen(buf) + 1));
  std_memcpy(man -> gxp_hostname, buf, sizeof(char) * (strlen(buf) + 1));

  man -> peer_hostnames = (char**) std_malloc(sizeof(char*) * man -> gxp_num_execs);
  man -> peer_eps = std_malloc(sizeof(inet_ep_t) * man -> gxp_num_execs);

  man -> rfp = std_fdopen(GXP_READ_FD, "r");
  man -> wfp = std_fdopen(GXP_WRITE_FD, "w");

  man ->ep_prio_pats = NULL;
  man ->ep_prio_count = 0;

  return man;
}

/**
   Initializes the GXP interface. This must be called before any GXP operations.
   \param man gxp interface instance
*/
void
gxp_man_init(gxp_man_t man){
  int i, idx;
  int peer_idx;
  char buff[1024];
  char *hostname;
  int **conn_mat;

  /* exchange hostnames */
  fprintf(man -> wfp, "%d %s\n", man -> gxp_idx, man -> gxp_hostname);
  fflush(man -> wfp);  
  for(i = 0; i < man -> gxp_num_execs; i++){
    fscanf(man -> rfp, "%d %s", &peer_idx, buff);

    hostname = (char*) std_malloc(sizeof(char) * (strlen(buff) + 1));
    std_memcpy(hostname, buff, sizeof(char) * (strlen(buff) + 1));
    man -> peer_hostnames[peer_idx] = hostname;
  }

  conn_mat = (int**)std_malloc(sizeof(int*) * man -> gxp_num_execs);
  for(idx = 0; idx < man -> gxp_num_execs; idx++)
    conn_mat[idx] = (int*)std_calloc(man -> gxp_num_execs, sizeof(int));
  man -> conn_mat = conn_mat;
}

/**
   Returns GXP_HOSTNAME environment variable.
   \param man gxp interface instance
   \return hostname in string
*/
const char*
gxp_man_hostname(gxp_man_t man){
  return man -> gxp_hostname;
}
  
/**
   Returns GXP_EXEC_IDX environment variable.
   \param man gxp interface instance
   \return the index of this GXP instance [0, GXP_NUM_EXECS)
*/
int
gxp_man_peer_id(gxp_man_t man){
  return man -> gxp_idx;
}
  
/**
   Returns GXP_NUM_EXECS environment variable.
   \param man gxp interface instance
   \return the number of GXP instances in this application
*/
int
gxp_man_num_peers(gxp_man_t man){
  return man -> gxp_num_execs;
}
  
/**
   In case a given host is multi-homing (multiple networks),
   there can be confusion as to what interface is used for other hosts to connect.
   This allows the user to specify the interface to publish to other hosts to connect.
   
   \param man       gxp interface instance
   \param prio_pats An array of string prefixes of inet addresses, in order of priority. (e.g.: {"192.168", "10.5"})
   \param npats     The array length
*/
void
gxp_man_set_iface_prio(gxp_man_t man, const char **prio_pats, int npats){
  man -> ep_prio_pats = prio_pats;
  man -> ep_prio_count = npats;
}

/**
   Destroy the GXP interface node.
   Before doing this, gxp_man_sync() should be performed.
   No GXP operations should be perfomed after this, or the behavior is undefined.
   
   \param man gxp interface instance
*/
void
gxp_man_destroy(gxp_man_t man){
  int idx;
  std_fclose(man -> rfp);
  std_fclose(man -> wfp);

  //destroy all endpoints
  for(idx= 0; idx< man -> gxp_num_execs; idx++){
    std_free(man -> peer_hostnames[idx]);
    inet_ep_destroy(man -> peer_eps[idx]);
    std_free(man -> conn_mat[idx]);
  }

  std_free(man -> peer_hostnames);
  std_free(man -> peer_eps);
  std_free(man -> conn_mat);
  std_free(man -> gxp_hostname);
  
  std_free(man);
}

/**
   Barrier synchronization operation among all GXP nodes.
   
   \param man gxp interface instance
*/
void
gxp_man_sync(gxp_man_t man){
  int i;
  char buf[1024];
  fprintf(man -> wfp, "GXP_MAN_SYNC\n");
  fflush(man -> wfp);
  for(i = 0; i < man -> gxp_num_execs; i++){
    fscanf(man -> rfp, "%s", buf);
    assert(!strcmp(buf, "GXP_MAN_SYNC"));
  }
}

static void
gxp_man_exchange_ports(gxp_man_t man, int my_port){
  inet_iface_t my_iface;
  int i;

  int peer_idx;
  char buff[1024];
  int peer_port;

  const char **prio_pats = man -> ep_prio_pats;
  int npats = man -> ep_prio_count;

  /* exchange endpoints */
  if((my_iface = inet_iface_get_my_iface(prio_pats, npats)) == NULL){
    fprintf(stderr, "%d %s: could not get any suitable inet iface\n", man -> gxp_idx, man -> gxp_hostname);
    fflush(stderr);
    exit(1);
  }
  /* printf("%s %s\n", man -> gxp_hostname, inet_iface_in_addr_str(my_iface));fflush(stdout); */
  fprintf(man -> wfp, "%d %s %d\n", man -> gxp_idx, inet_iface_in_addr_str(my_iface), my_port);
  fflush(man -> wfp);
  for(i = 0; i < man -> gxp_num_execs; i++){
    fscanf(man -> rfp, "%d %s %d", &peer_idx, buff, &peer_port);

    man -> peer_eps[peer_idx] = inet_ep_create_by_str(buff, peer_port);
  }
  
  inet_iface_destroy(my_iface);
}

static const int**
gxp_man_exchange_conn_info(gxp_man_t man, dlfree_comm_node_t comm, int *plan){
  int idx;
  int i, j;
  int src_idx, dst_idx, nconns, dist;
  for(idx = 0; idx < man -> gxp_num_execs; idx++){
    if(plan[idx] == GXP_CONNECT_YES){
      dist = (int)(dlfree_comm_node_peer_rtt(comm, idx) * 1e6);
      /* printf("READ %d -> %d %f\n", man -> gxp_idx, idx, dlfree_comm_node_peer_rtt(comm, idx));fflush(stdout); */
    }
    else
      dist = GXP_NO_RTT;
    
    fprintf(man -> wfp, "%d %d %d\n", man -> gxp_idx, idx, dist);
    fflush(man -> wfp);
  }
  for(i = 0; i < man -> gxp_num_execs; i++){
    for(j = 0; j < man -> gxp_num_execs; j++){
      fscanf(man -> rfp, "%d %d %d", &src_idx, &dst_idx, &dist);
      man -> conn_mat[src_idx][dst_idx] = dist;
    }
  }

  nconns = 0;
  for(i = 0; i < man -> gxp_num_execs; i++){
    for(j = i + 1; j < man -> gxp_num_execs; j++){
      if(man -> conn_mat[i][j] != GXP_NO_RTT)
	nconns ++;
    }
  }

  if(man -> gxp_idx == 0){
    printf("%d connections established...\n", nconns);fflush(stdout);
  }
  
  return (const int**)man -> conn_mat;
}

static const int**
gxp_man_connect(gxp_man_t man, dlfree_comm_node_t comm, int *plan){
  int idx;
  inet_ep_t ep;
  const char* addr;
  int port;
  int dst_id;

  unsigned long *handles = std_calloc(man -> gxp_num_execs, sizeof(unsigned long));

  /* exchange endpoint information */
  int my_port = dlfree_comm_node_listen_port(comm);
  gxp_man_exchange_ports(man, my_port);

  /* do async. connect to all planned peers */
  for(idx = 0; idx < man -> gxp_num_execs; idx++){
    if(plan[idx] != GXP_CONNECT_YES)
      continue;
    
    ep = man -> peer_eps[idx];
    addr = inet_iface_in_addr_str(inet_ep_iface(ep));
    port = inet_ep_port(ep);

/*     printf("%d: connecting to %d\n", man -> gxp_idx, idx);fflush(stdout); */
    handles[idx] = dlfree_comm_node_async_connect(comm, addr, port);
  }

  /* get connect() status from all attempted peers */
  for(idx = 0; idx < man -> gxp_num_execs; idx ++){
    if(plan[idx] != GXP_CONNECT_YES)
      continue;
    
    if(dlfree_comm_node_connect_wait(comm, handles[idx], &dst_id) != 0){
      plan[idx] = GXP_CONNECT_FAIL;
      fprintf(stderr, "%d: connect to peer fail: %d\n", man -> gxp_idx, idx);
    }

  }
  std_free(handles);

  return gxp_man_exchange_conn_info(man, comm, plan);
}

/**
   GXP operation to establish connect among other GXP nodes.
   All GXP nodes will connect to all other nodes.
   GXP_NUM_EXECS * (GXP_NUM_EXECS - 1) connections will be established.
   
   \param man  interface instance
   \param comm node communicator instance
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
const int**
gxp_man_connect_all(gxp_man_t man, dlfree_comm_node_t comm){
  int idx;
  int *plan = std_calloc(man -> gxp_num_execs, sizeof(int));
  const int **conn_mat;
  memset(plan, GXP_CONNECT_NO, sizeof(int) * man -> gxp_num_execs);

  if(man -> gxp_idx == 0){
    printf("connect all\n");fflush(stdout);
  }
  
  /* connect to all peers with larger idx */
  for(idx = man -> gxp_idx + 1; idx < man -> gxp_num_execs; idx++){
    plan[idx] = GXP_CONNECT_YES;
  }

  conn_mat = gxp_man_connect(man, comm, plan);

  std_free(plan);
  return conn_mat;
}

/**
   GXP operation to establish connect among other GXP nodes.
   Each node will decide who to connect to randomly
   
   \param man     gxp interface instance
   \param comm    node communicator instance
   \param density density of the graph with values [0, 1]. Where 1 means a fully connected graph.
   \param seed    srand() seed.
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
const int**
gxp_man_connect_random(gxp_man_t man, dlfree_comm_node_t comm, float density, int seed){
  int idx;
  int *plan = std_calloc(man -> gxp_num_execs, sizeof(int));
  const int **conn_mat;
  memset(plan, GXP_CONNECT_NO, sizeof(int) * man -> gxp_num_execs);

  if(man -> gxp_idx == 0){
    printf("connect random graph: density: %.3f seed: %d\n", density, seed);fflush(stdout);
  }
  
  /* connect to some peers with larger idx */
  srand(seed + man -> gxp_idx);
/*   printf("%d: CONNECT: [", man -> gxp_idx); */
  for(idx = man -> gxp_idx + 1; idx < man -> gxp_num_execs; idx++){
    if((float)rand() / RAND_MAX < density){
      plan[idx] = GXP_CONNECT_YES;
/*       printf("%d, ", idx); */
    }
  }
/*   printf("]\n");fflush(stdout); */

  conn_mat = gxp_man_connect(man, comm, plan);

  std_free(plan);
  return conn_mat;
}

/**
   GXP operation to establish connect among other GXP nodes.
   Each node will connect to the node with (GXP_EXEC_IDX + 1) % GXP_NUM_EXECS.
   A total of GXP_NUM_EXECS connections will be established.
   
   \param man     gxp interface instance
   \param comm    node communicator instance
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
const int**
gxp_man_connect_ring(gxp_man_t man, dlfree_comm_node_t comm){
  int *plan = std_calloc(man -> gxp_num_execs, sizeof(int));
  const int **conn_mat;
  memset(plan, GXP_CONNECT_NO, sizeof(int) * man -> gxp_num_execs);

  if(man -> gxp_idx == 0){
    printf("connect ring\n");fflush(stdout);
  }
  
  /* connect to next peer */
  plan[(man -> gxp_idx + 1) % man -> gxp_num_execs] = GXP_CONNECT_YES;

  conn_mat = gxp_man_connect(man, comm, plan);

  std_free(plan);
  return conn_mat;
}

/**
   GXP operation to establish connect among other GXP nodes.
   Each node will connect to the node with GXP_EXEC_IDX + 1.
   A total of GXP_NUM_EXECS - 1 connections will be established.
   
   \param man     gxp interface instance
   \param comm    node communicator instance
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
const int**
gxp_man_connect_lipne(gxp_man_t man, dlfree_comm_node_t comm){
  int *plan = std_calloc(man -> gxp_num_execs, sizeof(int));
  const int **conn_mat;
  memset(plan, GXP_CONNECT_NO, sizeof(int) * man -> gxp_num_execs);

  if(man -> gxp_idx == 0){
    printf("connect line\n");fflush(stdout);
  }
  
  /* connect to next peer, only if not tail peer */
  if(man -> gxp_idx != man -> gxp_num_execs - 1)
    plan[(man -> gxp_idx + 1) % man -> gxp_num_execs] = GXP_CONNECT_YES;

  conn_mat = gxp_man_connect(man, comm, plan);

  std_free(plan);
  return conn_mat;
}

/**
   GXP operation to establish connect among other GXP nodes.
   Cluster-aware topology scheme. Nodes within the same cluster will
   decide to connect to each other randomly. A fixed number of connections will
   be established between clusters.
   
   \param man     gxp interface instance
   \param comm    node communicator instance
   \param prefix  hostname prefix length used to determine if 2 nodes are in the same cluster
   \param ngates  number of connections that will be initiated from one cluster to another
   \param density density of the graph with values [0, 1]. Where 1 means a fully connected graph.
   \param seed    srand() seed.
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
const int**
gxp_man_connect_with_gateway(gxp_man_t man, dlfree_comm_node_t comm, int prefix, int ngates, float density, int seed){
  int idx, jdx, gateway;
  int *plan = std_calloc(man -> gxp_num_execs, sizeof(int));
  const int **conn_mat;
  memset(plan, GXP_CONNECT_NO, sizeof(int) * man -> gxp_num_execs);

  if(man -> gxp_idx == 0){
    printf("connect with gateway: ngates: %d density: %.3f seed: %d\n", ngates, density, seed);fflush(stdout);
  }

  /* elect gateway node */
  gateway = ngates;
  for(idx = 0; idx < man -> gxp_idx; idx ++){
    if(gxp_man_same_cluster(man, man -> gxp_idx, idx, prefix)){
      gateway --;
      if(gateway == 0) break;
    }
  }

  /* connect to inter-cluster gateways with larger idx*/
  if(gateway){
    printf("%d: i am gateway: %s\n", man -> gxp_idx, man -> gxp_hostname);fflush(stdout);
    for(idx = man -> gxp_idx + 1; idx < man -> gxp_num_execs; idx ++)
      plan[idx] = GXP_CONNECT_YES;
    /* eliminate all non-gateway nodes */
    for(idx = man -> gxp_idx + 1; idx < man -> gxp_num_execs; idx ++){
      gateway = ngates;
      for(jdx = 0; jdx < idx; jdx ++){
	if(gxp_man_same_cluster(man, jdx, idx, prefix)){
	  gateway --;
	  if(gateway == 0){
	    plan[idx] = GXP_CONNECT_NO;
	    break;
	  }
	}
      }
/*       if(gateway) */
/* 	printf("%d: connecting to %d: %s\n", man -> gxp_idx, idx, man -> peer_hostnames[idx]);fflush(stdout); */
    }
  }
  /* connect to all intra-cluster peers with larger idx*/
  srand(seed + man -> gxp_idx);
  for(idx = man -> gxp_idx + 1; idx < man -> gxp_num_execs; idx ++){
    if(gxp_man_same_cluster(man, man -> gxp_idx, idx, prefix))
      if((float)rand() / RAND_MAX < density)
	plan[idx] = GXP_CONNECT_YES;
  }

  conn_mat = gxp_man_connect(man, comm, plan);

  std_free(plan);
  return conn_mat;
}

/**
   GXP operation to establish connect among other GXP nodes.
   Locality-aware topology scheme. 
   All nodes will sort other nodes based on proximity to itself.
   Each node will connect to random alpha nodes with indexes [alpha * 2^i, alpha * 2^(i+1)].
   Currently, the proximity is determined by a user provided XML file with topology information.
   
   \param man      gxp interface instance
   \param comm     node communicator instance
   \param filename XML topology filename used to determine proximity
   \param alpha    parameter to determine density of connection topology
   \param seed     srand() seed.
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
const int**
gxp_man_connect_locality_aware(gxp_man_t man, dlfree_comm_node_t comm, const char* filename, int alpha, int seed){
  xml_topology_t xml_top;
  xml_topology_parser_t parser = xml_topology_parser_create();
  xml_top_node_vector_t path = xml_top_node_vector_create(1);
  
  int idx, src_idx, dst_idx, i, j, k, lim;
  float len, width;
  int npeers = man -> gxp_num_execs - 1;
  float *dist = std_calloc(npeers, sizeof(float));
  int *idxs = std_calloc(npeers, sizeof(int));
  const int **conn_mat;
  int *plan = std_calloc(man -> gxp_num_execs, sizeof(int));
  memset(plan, GXP_CONNECT_NO, sizeof(int) * man -> gxp_num_execs);

  assert(alpha > 0);

  if(man -> gxp_idx == 0){
    printf("connect locality aware alpha: %d seed: %d\n", alpha, seed);fflush(stdout);
  }

  /* parse xml and figure out distance to all nodes */
  xml_top = xml_topology_parser_run(parser, filename);
  /* randomize metric */
  xml_topology_randomize_edge_width(xml_top, seed);
  
  xml_topology_parser_destroy(parser);
  for(idx = 0, i = 0; idx < man -> gxp_num_execs; idx ++){
    if(idx == man -> gxp_idx){
      continue;
    }
    else{
      xml_topology_traverse(xml_top, man -> gxp_hostname, man -> peer_hostnames[idx], path, &len, &width);
      xml_top_node_vector_clear(path);
    }
    dist[i] = len;
    idxs[i] = idx;
    i++;
  }

  srand(seed + man -> gxp_idx);
  
  /* bubble sort: according to shorter dist */
  /* but first need to randomize to remove any bias */
  shuffle_and_sort_pairs(dist, idxs, npeers);

  /* need to synchronize before going on */
  gxp_man_sync(man);
  
  /* decide to whom to connect according to distance */
  for(i = 0, lim = alpha; i < npeers; lim *= 2){
    j = Min(i + lim, npeers);
    /* randomize */
    shuffle_array(idxs + i, j - i);
    /* in the range of [i, j) try to select alpha nodes */
    /* boundary condition: when range of [i, j) < lim */
    for(k = i; k < j; k ++){
      if( lim == (j - i) && k >= i + alpha) break;
      if( lim == (j - i) || (float)rand() / RAND_MAX < (float)alpha / lim){
	idx = idxs[k];
	if(idx > man -> gxp_idx)
	  plan[idx] = GXP_CONNECT_YES;
	else{ /* if smaller idx, have peer connect back */
	  fprintf(man -> wfp, "%d %d\n", idx, man -> gxp_idx);
	}
      }
    }
    i = j;
  }
  /* notify that i have finished making all requests */
  fprintf(man -> wfp, "%d %d\n", -1, man -> gxp_idx);
  fflush(man -> wfp);

  /* read all connect back requests */
  /* and plan connection if connect to self */
  for(i = 0; i < man -> gxp_num_execs;){
    fscanf(man -> rfp, "%d %d", &src_idx, &dst_idx);
    if(src_idx == -1)
      i++;
    if(src_idx == man -> gxp_idx)
      plan[dst_idx] = GXP_CONNECT_YES;
  }

  conn_mat = gxp_man_connect(man, comm, plan);

  xml_top_node_vector_destroy(path);
  xml_topology_destroy(xml_top);
  std_free(plan);
  std_free(dist);
  std_free(idxs);
  return conn_mat;
}

static void
gxp_man_conn_stats(gxp_man_t man, const int **conn_mat){
  int src, dst;
  int intra_c = 0;
  int inter_c = 0;
  int total = 0;
  for(src = 0; src < man -> gxp_num_execs; src ++){
    for(dst = src + 1; dst < man -> gxp_num_execs; dst ++){
      if(conn_mat[src][dst]){
	if(gxp_man_same_cluster(man, src, dst, GXP_CLUSTER_NAME_PREFIX))
	  intra_c ++;
	else
	  inter_c ++;
	total ++;
      }
    }
  }
  printf("intra cluster links: %d/%d %.3f\n", intra_c, total, (float)intra_c / total);
  printf("inter cluster links: %d/%d %.3f\n", inter_c, total, (float)inter_c / total);
  fflush(stdout);
}

static void
gxp_man_rt_stats(gxp_man_t man, gxp_router_t router, overlay_rtable_t *rts){
  int idx;
  overlay_rtable_vector_t rtable_vec = overlay_rtable_vector_create(1);
  for(idx = 0; idx < man -> gxp_num_execs; idx++){
    overlay_rtable_vector_add(rtable_vec, rts[idx]);
    overlay_rtable_print_with_name(rts[idx], (const char**)man -> peer_hostnames);
  }

  overlay_rtable_stats_calc_hops(rtable_vec, man -> gxp_num_execs);
  overlay_rtable_stats_print_band_histo(rtable_vec, man -> gxp_num_execs, router -> xml_top, router -> graph, 10);
  overlay_rtable_stats_calc_node_hotspot(rtable_vec, man -> gxp_num_execs);
  overlay_rtable_stats_calc_edge_hotspot(rtable_vec, man -> gxp_num_execs);
  overlay_rtable_stats_calc_roundabout(rtable_vec, man -> gxp_num_execs, router -> xml_top, router -> graph);

  overlay_rtable_vector_destroy(rtable_vec);
}

/**
   GXP operation to collectively compute each nodes routing table, and exchange it among all nodes.
   
   \param man          gxp interface instance
   \param comm         node communicator instance
   \param xml_filename XML topology filename used to determine proximity
   \param rttype       specify the type of routing (gxp_routing_type)
   \param seed         srand() seed.
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
void
gxp_man_compute_rt(gxp_man_t man, dlfree_comm_node_t comm, const char* xml_filename, int rttype, int seed){
  gxp_router_t router = gxp_router_create(xml_filename, seed);
  overlay_rtable_t rt = gxp_router_run(router, man, (const int**)man -> conn_mat, rttype, seed);

  /* comm_node_t and dlfree_comm_node_t are the same */
  comm_node_exchange_rt((comm_node_t)comm, rt, man -> gxp_num_execs);

  if(man -> gxp_idx == 0){
    printf("gxp_man_compute_rt: exchanged routing tables\n");fflush(stdout);
  }

  if(man -> gxp_idx == 0){
    gxp_man_conn_stats(man, (const int**)man -> conn_mat);
    /* gxp_man_rt_stats(man, router, comm -> rts); */
  }

  gxp_router_destroy(router);

  gxp_man_sync(man);
}

/**
   GXP operation to collectively measure latency between all GXP node pairs.
   
   \param man  interface instance
   \param comm node communicator instance
   \param len  size of each ping message
   \param iter number of iterations per node pair
   
   \return 2D bit matrix. if (i, j) = 1, node i and node j are connected.
*/
void
gxp_man_ping_pong(gxp_man_t man, dlfree_comm_node_t comm, long len, int iter){
  const float MAX_RTT = 1000.0;
  void *buff;
  struct timeval tv0, tv1;
  double min_dt, dt;
  int src_idx, dst_idx, it;

  if(man -> gxp_idx == 0){
    printf("gxp_man_ping_pong: %ld [B]\n", len);fflush(stdout);
  }

  buff = std_calloc(len, sizeof(char));

  gxp_man_sync(man);

  for(src_idx = 0; src_idx < man -> gxp_num_execs; src_idx++){
    for(dst_idx = 0; dst_idx < man -> gxp_num_execs; dst_idx++){
      if(dst_idx != man -> gxp_idx && src_idx != man -> gxp_idx) continue;
      if(dst_idx == src_idx) continue;
      
      min_dt = MAX_RTT;
      
      for(it = 0; it < iter; it++){
	if(man -> gxp_idx == src_idx){
	  assert(gettimeofday(&tv0, NULL) == 0);
	  
	  dlfree_comm_node_send_data(comm, dst_idx, buff, len);
	  wait_messages(comm, 1);
	  
	  assert(gettimeofday(&tv1, NULL) == 0);
	  dt = timeval_diff(&tv0, &tv1);
	  min_dt = dt < min_dt ? dt : min_dt;
	}
	else if(man -> gxp_idx == dst_idx){
	  wait_messages(comm, 1);
	  dlfree_comm_node_send_data(comm, src_idx, buff, 1); /* just an ack */
	}
      }
      
      if(man -> gxp_idx == src_idx)
	fprintf(stdout, "ping-pong, %d, ->, %d, %s, ->, %s, %.3f, [MB/s], %.3f, [ms]\n",
		src_idx, dst_idx, man -> peer_hostnames[src_idx], man -> peer_hostnames[dst_idx],
		len / min_dt / (1000 * 1000), min_dt * 1000);fflush(stdout);
    }
    gxp_man_sync(man);
  }
  
  std_free(buff);
}
