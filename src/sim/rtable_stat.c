#include <assert.h>
#include <stdio.h>
#include <limits.h>

#include <std/std.h>
#include <struct/rtable.h>
#include <xml/topology.h>
#include <graph/overlay.h>
#include "sim/rtable_stat.h"

void
overlay_rtable_stats_calc_hops(overlay_rtable_vector_t rt_vec, int nnodes){
  int i, pid;
  int maxhops = 0, hops;
  long sum; int count;
  int *histo = std_calloc(nnodes, sizeof(int));
  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;

  maxhops = 0; count = 0; sum = 0;
  for(i = 0; i < overlay_rtable_vector_size(rt_vec); i++){
    rt = overlay_rtable_vector_get(rt_vec, i);
    for(pid = 0; pid < rt -> nentry; pid++){
      if(pid == rt -> srcpid) continue;
      rt_entry = rt -> entries[pid];
      hops = rt_entry -> hops;
      histo[hops] ++;
      maxhops = maxhops > hops ? maxhops : hops;
      sum += hops;
      count ++;
    }
  }

  assert(maxhops < nnodes);
  

  printf("HOPS max: %d\n", maxhops);
  printf("HOPS avg: %.6f\n", ((float)sum) / count);
  for(i = 1; i < maxhops + 1; i++){
    printf("HOPS: %d: %d\n", i, histo[i]);
  }
  
  std_free(histo);
}

void
overlay_rtable_stats_print_band_histo(overlay_rtable_vector_t rt_vec, int nnodes, xml_topology_t xml_top, overlay_graph_t graph, int nbins){
  int i, dstpid, found;
  float len, width, ratio;
  float max, min, sum; int count;
  xml_top_node_vector_t path = xml_top_node_vector_create(1);
  overlay_node_t node;

  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;
  
  int *histo = std_calloc(nbins + 1, sizeof(int));
  char **peer_names = std_calloc(nnodes, sizeof(char*));

  /* first get pid -> hostname mapping */
  for(i = 0; i < nnodes; i++){
    node = overlay_node_vector_get(graph -> hosts, i);
    peer_names[node -> pid] = node -> name;
  }

  max = 0.0; min = 1.0, sum = 0.0; count = 0;
  for(i = 0; i < overlay_rtable_vector_size(rt_vec); i++){
    rt = overlay_rtable_vector_get(rt_vec, i);

    /* overlay_rtable_print_with_name(rt, (const char**)peer_names); */
    
    for(dstpid = 0; dstpid < rt -> nentry; dstpid++){
      if(dstpid == rt -> srcpid) continue;

      rt_entry = rt -> entries[dstpid];
      found = xml_topology_traverse(xml_top, peer_names[rt -> srcpid], peer_names[dstpid], path, &len, &width); assert(found);

      /* find ratio to max. capacity on actual topology */
      /* because we have entry width stored as int, compare as int values */
      ratio = (float)rt_entry -> width / (int)width;
      histo[(int)(ratio * nbins)] ++;

      max = max > ratio ? max : ratio;
      min = min < ratio ? min : ratio;
      sum += ratio;
      count ++;

/*       if(ratio < 0.5){ */
/* 	printf("%.6f ", ratio); overlay_rtable_entry_print_with_name(rt_entry, (const char**)peer_names); */
/* 	printf("\n"); */
/*       } */
      
      xml_top_node_vector_clear(path);
    }
  }

  printf("BANDCAP min: %.6f\n", min);
  printf("BANDCAP max: %.6f\n", max);
  printf("BANDCAP avg: %.6f\n", sum / count);
  
  for(i = 0; i <= nbins; i++){
    printf("BANDCAP: %.3f: %d\n", i * 1.0 / nbins, histo[i]);
  }

  /* clean up */
  xml_top_node_vector_destroy(path);
  std_free(histo);
  std_free(peer_names);
}

int**
overlay_rtable_stats_calc_band(overlay_rtable_vector_t rt_vec, int nnodes, xml_topology_t xml_top, overlay_graph_t graph){
  int i, dstpid, found;
  float len, width, ratio;
  xml_top_node_vector_t path = xml_top_node_vector_create(1);
  overlay_node_t node;

  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;
  
  char **peer_names = std_calloc(nnodes, sizeof(char*));

  /* initialize connection requests */
  int **conn_req = std_calloc(nnodes, sizeof(int*));
  for(i = 0; i < nnodes; i++)
    conn_req[i] = std_calloc(nnodes, sizeof(int));

  /* first get pid -> hostname mapping */
  for(i = 0; i < nnodes; i++){
    node = overlay_node_vector_get(graph -> hosts, i);
    peer_names[node -> pid] = node -> name;
  }

  for(i = 0; i < overlay_rtable_vector_size(rt_vec); i++){
    rt = overlay_rtable_vector_get(rt_vec, i);

    for(dstpid = 0; dstpid < rt -> nentry; dstpid++){
      if(dstpid == rt -> srcpid) continue;

      rt_entry = rt -> entries[dstpid];
      found = xml_topology_traverse(xml_top, peer_names[rt -> srcpid], peer_names[dstpid], path, &len, &width); assert(found);

      /* find ratio to max. capacity on actual topology */
      /* issue connection request if bandwidth is bad... */
      ratio = (float)rt_entry -> width / (int)width;
      if(ratio < 0.5){
	printf("%.6f ", ratio); overlay_rtable_entry_print_with_name(rt_entry, (const char**)peer_names);
	printf("\n");
	
	conn_req[rt -> srcpid][dstpid] = 1;
	conn_req[dstpid][rt -> srcpid] = 1;
      }
      
      xml_top_node_vector_clear(path);
    }
  }

  /* clean up */
  xml_top_node_vector_destroy(path);
  std_free(peer_names);

  return conn_req;
}

void
overlay_rtable_stats_calc_node_hotspot(overlay_rtable_vector_t rt_vec, int nnodes){
  const int BIN = 100;
  int i, idx, dstpid;
  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;

  int c, max, min, nbin;
  int *histo;
  int *usage = std_calloc(nnodes, sizeof(int));

  max = 0; min = INT_MAX;
  for(i = 0; i < overlay_rtable_vector_size(rt_vec); i++){
    rt = overlay_rtable_vector_get(rt_vec, i);

    for(dstpid = 0; dstpid < rt -> nentry; dstpid++){
      if(dstpid == rt -> srcpid) continue;

      rt_entry = rt -> entries[dstpid];
      /* just check forwarding nodes */
      for(idx = 1; idx < rt_entry -> hops; idx ++){
	c = ++ usage[rt_entry -> path[idx]];
	max = max > c ? max : c;
	min = min < c ? min : c;
      }
    }
  }

  printf("NODE HOTSPOT min: %d\n", min);
  printf("NODE HOTSPOT max: %d\n", max);
  
  nbin = (max / BIN) + 1;
  histo = std_calloc(nbin, sizeof(int));
  for(idx = 0; idx < nnodes; idx++){
    printf("NODE HOTSPOT: [%d]: %d\n", idx, usage[idx]);
    histo[usage[idx] / BIN] ++;
  }
  
  for(i = 0; i < nbin; i++){
    printf("NODE HOTSPOT: %d: %d\n", i * BIN, histo[i]);
  }
  

  /* clean up */
  std_free(histo);
  std_free(usage);
}

void
overlay_rtable_stats_calc_edge_hotspot(overlay_rtable_vector_t rt_vec, int nnodes){
  const int BIN = 1;
  int i, idx, jdx, dstpid;
  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;

  int c, max, min, nbin;
  int *histo;
  int **usage = std_calloc(nnodes, sizeof(int*));
  for(i = 0; i < nnodes; i++)
    usage[i] = std_calloc(nnodes, sizeof(int));

  max = 0; min = INT_MAX;
  for(i = 0; i < overlay_rtable_vector_size(rt_vec); i++){
    rt = overlay_rtable_vector_get(rt_vec, i);

    for(dstpid = 0; dstpid < rt -> nentry; dstpid++){
      if(dstpid == rt -> srcpid) continue;

      rt_entry = rt -> entries[dstpid];

      /* just each edge in path */
      for(idx = 0; idx < rt_entry -> hops; idx ++){
	c = ++ usage[rt_entry -> path[idx]][rt_entry -> path[idx + 1]];
	max = max > c ? max : c;
	min = min < c ? min : c;
      }
    }
  }

  nbin = (max / BIN) + 1;
  histo = std_calloc(nbin, sizeof(int));
  for(idx = 0; idx < nnodes; idx++){
    for(jdx = 0; jdx < nnodes; jdx++){
      if(usage[idx][jdx] > 0)
	/* printf("EDGE HOTSPOT: [%d][%d] %d\n", idx, jdx, usage[idx][jdx]); */
      histo[usage[idx][jdx] / BIN] ++;
    }
  }
  
  for(i = 0; i < nbin; i++){
    printf("EDGE HOTSPOT: %d: %d\n", i * BIN, histo[i]);
  }
/*   printf("EDGE HOTSPOT min %d\n", min); */
/*   printf("EDGE HOTSPOT max %d\n", max); */

  /* clean up */
  std_free(histo);
  for(i = 0; i < nnodes; i++)
    std_free(usage[i]);
  std_free(usage);
}

void
overlay_rtable_stats_calc_roundabout(overlay_rtable_vector_t rt_vec, int nnodes, xml_topology_t xml_top, overlay_graph_t graph){
  const int BIN = 1;
  int i, j, dstpid, found, hops, excess, max, min, nbin;
  float len, width;
  xml_top_node_vector_t **real_paths = std_malloc(sizeof(xml_top_node_vector_t*) * nnodes);
  xml_top_node_vector_t path;
  overlay_node_t node;

  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;
  
  int *histo;
  char **peer_names = std_calloc(nnodes, sizeof(char*));
  int **usage = std_calloc(nnodes, sizeof(int*));
  for(i = 0; i < nnodes; i++)
    usage[i] = std_calloc(nnodes, sizeof(int));

  /* first get pid -> hostname mapping */
  for(i = 0; i < nnodes; i++){
    node = overlay_node_vector_get(graph -> hosts, i);
    peer_names[node -> pid] = node -> name;
  }

  /* populate paths according to underlying topology */
  for(i = 0; i < nnodes; i++){
    real_paths[i] = std_calloc(nnodes, sizeof(xml_top_node_vector_t));
    for(j = 0; j < nnodes; j++){
      if(i == j) continue;
      path = xml_top_node_vector_create(1);
      found = xml_topology_traverse(xml_top, peer_names[i], peer_names[j], path, &len, &width); assert(found);
      real_paths[i][j] = path;
    }
  }

  max = 0; min = INT_MAX;
  for(i = 0; i < overlay_rtable_vector_size(rt_vec); i++){
    rt = overlay_rtable_vector_get(rt_vec, i);

    for(dstpid = 0; dstpid < rt -> nentry; dstpid++){
      if(dstpid == rt -> srcpid) continue;

      rt_entry = rt -> entries[dstpid];

/*       printf("R ["); */
      hops = 0;
      for(j = 0; j < rt_entry -> hops; j++){
	path = real_paths[ rt_entry -> path[j] ][ rt_entry -> path[j+1] ];
	hops += (xml_top_node_vector_size(path) - 1);
/* 	for(k = 1; k < xml_top_node_vector_size(path); k++){ */
/* 	  printf("%s, ", xml_top_node_vector_get(path, k) -> name); */
/* 	} */
      }
/*       printf("]\n"); */
      
      path = real_paths[rt -> srcpid][dstpid];
      excess = hops - (xml_top_node_vector_size(path) - 1);
/*       printf("D ["); */
/*       for(k = 1; k < xml_top_node_vector_size(path); k++){ */
/* 	printf("%s, ", xml_top_node_vector_get(path, k) -> name); */
/*       } */
/*       printf("]\n"); */
/*       printf("excess: %d\n", excess); */

      usage[rt -> srcpid][dstpid] = excess;
      max = max > excess ? max : excess;
      min = min < excess ? min : excess;
    }
  }

  nbin = max / BIN + 1;
  histo = std_calloc(nbin, sizeof(int));
  for(i = 0; i < nnodes; i++)
    for(j = 0; j < nnodes; j++)
      histo[usage[i][j] / BIN ] ++;

  for(i = 0; i < nbin; i++){
    printf("EXCESS %d: %d\n", i * BIN, histo[i]);
  }
/*   printf("EXCESS min %d\n", min); */
/*   printf("EXCESS max %d\n", max); */

  /* clean up */
  for(i = 0; i < nnodes; i++){
    for(j = 0; j < nnodes; j++){
      if(i == j) continue;
      xml_top_node_vector_destroy(real_paths[i][j]);
    }
    std_free(real_paths[i]);
  }
  std_free(real_paths);

  for(i = 0; i < nnodes; i++)
    std_free(usage[i]);
  std_free(usage);
  
  std_free(histo);
  std_free(peer_names);
}
