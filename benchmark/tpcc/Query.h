//
// Created by Yi Lu on 7/19/18.
//

#pragma once

#include "benchmark/tpcc/Context.h"
#include "benchmark/tpcc/Random.h"
#include "common/FixedString.h"
#include <string>

namespace star {
namespace tpcc {

int32_t wid_to_granule_id(int32_t W_ID, const Context &context) {
  if (context.granules_per_partition == 1) {
    return 0;
  }
  auto W_NUM = context.partition_num;
  auto D_NUM = 10;
  return W_ID + context.granules_per_partition - (W_NUM + 1);
}

int32_t did_to_granule_id(int32_t D_ID, const Context &context) {
  if (context.granules_per_partition == 1) {
    return 0;
  }
  auto W_NUM = context.partition_num;
  auto D_NUM = 10;
  return D_ID + context.granules_per_partition - (W_NUM + 1) - (D_NUM + 1);
}

int32_t id_to_granule_id(int32_t ID, const Context &context) {
  if (context.granules_per_partition == 1) {
    return 0;
  }
  auto W_NUM = context.partition_num;
  auto D_NUM = 10;
  return ID % (context.granules_per_partition - (W_NUM + 1) - (D_NUM + 1));
}


struct NewOrderQuery {
  bool isRemote() {
    for (auto i = 0; i < O_OL_CNT; i++) {
      if (INFO[i].OL_SUPPLY_W_ID != W_ID) {
        return true;
      }
    }
    return false;
  }

  void unpack_transaction(const simpleTransaction& t){
    auto record_key = t.keys[2];
    this->W_ID = (record_key & RECORD_COUNT_W_ID_VALID) >>  RECORD_COUNT_W_ID_OFFSET;
    this->D_ID = (record_key & RECORD_COUNT_D_ID_VALID) >>  RECORD_COUNT_D_ID_OFFSET;
    DCHECK(this->D_ID >= 1);
    this->C_ID = (record_key & RECORD_COUNT_C_ID_VALID) >>  RECORD_COUNT_C_ID_OFFSET;
    DCHECK(this->C_ID >= 1);

    for(size_t i = 0 ; i < t.keys.size(); i ++ ){
      auto record_key = t.keys[i];
      if(i >= 3){
        INFO[i - 3].OL_I_ID = (record_key & RECORD_COUNT_OL_ID_VALID);
        INFO[i - 3].OL_SUPPLY_W_ID = 
          (record_key & RECORD_COUNT_W_ID_VALID) >>  RECORD_COUNT_W_ID_OFFSET;
      }
    }
  }
  void print(){
    LOG(INFO) << this->W_ID << " " << this->D_ID << " " << this->C_ID;
  }
  std::string print_str(){
    return std::to_string(this->W_ID) + " " + 
           std::to_string(this->D_ID) + " " + 
           std::to_string(this->C_ID) ;
  }
  int32_t W_ID;
  int32_t D_ID;
  int32_t C_ID;
  int8_t O_OL_CNT;

  struct NewOrderQueryInfo {
    int32_t OL_I_ID;
    int32_t OL_SUPPLY_W_ID;
    int8_t OL_QUANTITY;
  };

  NewOrderQueryInfo INFO[15];
  std::vector<uint64_t> record_keys; // for migration

  int32_t parts[2];
  int32_t granules[2][17];
  int32_t part_granule_count[2];
  int num_parts = 0;

  int32_t get_part(int i) {
    DCHECK(i < num_parts);
    return parts[i];
  }

  int32_t get_part_granule_count(int i) {
    DCHECK(i < num_parts);
    return part_granule_count[i];
  }

  int32_t get_part_granule(int i, int j) {
    DCHECK(i < num_parts);
    return granules[i][j];
  }

  int number_of_parts() {
    return num_parts;
  }
};

class makeNewOrderQuery {
public:
  NewOrderQuery operator()(const Context &context, int32_t W_ID,
                           Random &random) const {
    NewOrderQuery query;
    // W_ID is constant over the whole measurement interval
    query.W_ID = W_ID;

    query.num_parts = 1;

    query.parts[0] = W_ID - 1;
    query.part_granule_count[0] = query.part_granule_count[1] = 0;
    // The district number (D_ID) is randomly selected within [1 .. 10] from the
    // home warehouse (D_W_ID = W_ID).
    query.D_ID = random.uniform_dist(1, 10);

    // The non-uniform random customer number (C_ID) is selected using the
    // NURand(1023,1,3000) function from the selected district number (C_D_ID =
    // D_ID) and the home warehouse number (C_W_ID = W_ID).

    query.C_ID = random.non_uniform_distribution(1023, 1, 3000);

    // The number of items in the order (ol_cnt) is randomly selected within [5
    // .. 15] (an average of 10).

    query.O_OL_CNT = random.uniform_dist(5, 15);

    int rbk = random.uniform_dist(1, 100);

    for (auto i = 0; i < query.O_OL_CNT; i++) {

      // A non-uniform random item number (OL_I_ID) is selected using the
      // NURand(8191,1,100000) function. If this is the last item on the order
      // and rbk = 1 (see Clause 2.4.1.4), then the item number is set to an
      // unused value.

      bool retry;
      do {
        retry = false;
        query.INFO[i].OL_I_ID =
            random.non_uniform_distribution(8191, 1, 100000);
        for (int k = 0; k < i; k++) {
          if (query.INFO[k].OL_I_ID == query.INFO[i].OL_I_ID) {
            retry = true;
            break;
          }
        }
      } while (retry);

      if (i == query.O_OL_CNT - 1 && rbk == 1) {
        query.INFO[i].OL_I_ID = 0;
      }

      // The first supplying warehouse number (OL_SUPPLY_W_ID) is selected as
      // the home warehouse 90% of the time and as a remote warehouse 10% of the
      // time.
      int part_idx = -1;
      if (i == 0) {
        int x = random.uniform_dist(1, 100);
        if (x <= context.newOrderCrossPartitionProbability &&
            context.partition_num > 1) {
          int32_t OL_SUPPLY_W_ID = W_ID;
          while (OL_SUPPLY_W_ID == W_ID) {
            OL_SUPPLY_W_ID = random.uniform_dist(1, context.partition_num);
          }
          query.INFO[i].OL_SUPPLY_W_ID = OL_SUPPLY_W_ID;
          query.num_parts = 2;
          query.parts[0] = OL_SUPPLY_W_ID - 1;
          query.parts[1] = W_ID - 1;
          DCHECK(query.part_granule_count[1] == 0);
          query.granules[1][query.part_granule_count[1]++] = wid_to_granule_id(W_ID, context);
          part_idx = 0;
        } else {
          query.INFO[i].OL_SUPPLY_W_ID = W_ID;
          query.granules[0][query.part_granule_count[0]++] = wid_to_granule_id(W_ID, context);
          part_idx = 0;
        }
      } else {
        query.INFO[i].OL_SUPPLY_W_ID = W_ID;
        part_idx = query.num_parts == 2 ? 1: 0;
      }

      query.INFO[i].OL_QUANTITY = random.uniform_dist(1, 10);

      auto g = id_to_granule_id(query.INFO[i].OL_I_ID, context);
      bool exist = false;

      for (auto k = 0; k < query.part_granule_count[part_idx]; ++k) {
        if ((int)g == query.granules[part_idx][k]) {
          exist = true;
          break;
        }
      }
      if (exist == false) {
        query.granules[part_idx][query.part_granule_count[part_idx]++] = g;
      }
    }

    int part_idx = query.num_parts == 2 ? 1: 0;
    auto g = did_to_granule_id(query.D_ID, context);
    bool exist = false;

    for (auto k = 0; k < query.part_granule_count[part_idx]; ++k) {
      if ((int)g == query.granules[part_idx][k]) {
        exist = true;
        break;
      }
    }
    if (exist == false) {
      query.granules[part_idx][query.part_granule_count[part_idx]++] = g;
    }
    return query;
  }

  NewOrderQuery operator()(const Context &context, 
                           int32_t W_ID,
                           Random &random,
                           double cur_timestamp) const {

    NewOrderQuery query;
    // W_ID is constant over the whole measurement interval
    query.W_ID = W_ID;

    query.num_parts = 1;

    query.parts[0] = W_ID - 1;
    query.part_granule_count[0] = query.part_granule_count[1] = 0;
    // The district number (D_ID) is randomly selected within [1 .. 10] from the
    // home warehouse (D_W_ID = W_ID).
    query.D_ID = random.uniform_dist(1, 10);

    // The non-uniform random customer number (C_ID) is selected using the
    // NURand(1023,1,3000) function from the selected district number (C_D_ID =
    // D_ID) and the home warehouse number (C_W_ID = W_ID).

    query.C_ID = random.non_uniform_distribution(1023, 1, 3000);

    // The number of items in the order (ol_cnt) is randomly selected within [5
    // .. 15] (an average of 10).

    query.O_OL_CNT = random.uniform_dist(5, 15);

    int rbk = random.uniform_dist(1, 100);

    for (auto i = 0; i < query.O_OL_CNT; i++) {

      // A non-uniform random item number (OL_I_ID) is selected using the
      // NURand(8191,1,100000) function. If this is the last item on the order
      // and rbk = 1 (see Clause 2.4.1.4), then the item number is set to an
      // unused value.

      bool retry;
      do {
        retry = false;
        query.INFO[i].OL_I_ID =
            random.non_uniform_distribution(8191, 1, 100000);
        for (int k = 0; k < i; k++) {
          if (query.INFO[k].OL_I_ID == query.INFO[i].OL_I_ID) {
            retry = true;
            break;
          }
        }
      } while (retry);

      if (i == query.O_OL_CNT - 1 && rbk == 1) {
        query.INFO[i].OL_I_ID = 0;
      }

      // The first supplying warehouse number (OL_SUPPLY_W_ID) is selected as
      // the home warehouse 90% of the time and as a remote warehouse 10% of the
      // time.
      int part_idx = -1;
      if (i == 0) {
        int x = random.uniform_dist(1, 100);
        if (x <= context.newOrderCrossPartitionProbability &&
            context.partition_num > 1) {
          int32_t OL_SUPPLY_W_ID = W_ID;
          while (OL_SUPPLY_W_ID == W_ID) {
            OL_SUPPLY_W_ID = random.uniform_dist(1, context.partition_num);
          }
          query.INFO[i].OL_SUPPLY_W_ID = OL_SUPPLY_W_ID;
          query.num_parts = 2;
          query.parts[0] = OL_SUPPLY_W_ID - 1;
          query.parts[1] = W_ID - 1;
          DCHECK(query.part_granule_count[1] == 0);
          query.granules[1][query.part_granule_count[1]++] = wid_to_granule_id(W_ID, context);
          part_idx = 0;
        } else {
          query.INFO[i].OL_SUPPLY_W_ID = W_ID;
          query.granules[0][query.part_granule_count[0]++] = wid_to_granule_id(W_ID, context);
          part_idx = 0;
        }
      } else {
        query.INFO[i].OL_SUPPLY_W_ID = W_ID;
        part_idx = query.num_parts == 2 ? 1: 0;
      }

      query.INFO[i].OL_QUANTITY = random.uniform_dist(1, 10);

      auto g = id_to_granule_id(query.INFO[i].OL_I_ID, context);
      bool exist = false;

      for (auto k = 0; k < query.part_granule_count[part_idx]; ++k) {
        if ((int)g == query.granules[part_idx][k]) {
          exist = true;
          break;
        }
      }
      if (exist == false) {
        query.granules[part_idx][query.part_granule_count[part_idx]++] = g;
      }
    }

    int part_idx = query.num_parts == 2 ? 1: 0;
    auto g = did_to_granule_id(query.D_ID, context);
    bool exist = false;

    for (auto k = 0; k < query.part_granule_count[part_idx]; ++k) {
      if ((int)g == query.granules[part_idx][k]) {
        exist = true;
        break;
      }
    }
    if (exist == false) {
      query.granules[part_idx][query.part_granule_count[part_idx]++] = g;
    }
    return query;
}

  NewOrderQuery operator()(const std::vector<size_t>& keys) const {
    NewOrderQuery query;

    size_t size_ = keys.size();

    DCHECK(size_ == 13);
    // 
    auto c_record_key = keys[2];

    int32_t w_id = (c_record_key & RECORD_COUNT_W_ID_VALID) >> RECORD_COUNT_W_ID_OFFSET;

    
    int32_t d_id = (c_record_key & RECORD_COUNT_D_ID_VALID) >> RECORD_COUNT_D_ID_OFFSET;
    int32_t c_id = (c_record_key & RECORD_COUNT_C_ID_VALID) >> RECORD_COUNT_C_ID_OFFSET;

    query.W_ID = w_id;
    query.D_ID = d_id;
    DCHECK(query.D_ID >= 1);

    query.C_ID = c_id;
    DCHECK(query.C_ID >= 1);
    query.O_OL_CNT = 10; // random.uniform_dist(5, 10);

    for (auto i = 0; i < query.O_OL_CNT; i++) {
      auto cur_key = keys[3 + i];

      int32_t w_id = (cur_key & RECORD_COUNT_W_ID_VALID) >> RECORD_COUNT_W_ID_OFFSET;
      int32_t s_id = (cur_key & RECORD_COUNT_OL_ID_VALID);

      query.INFO[i].OL_I_ID = s_id;// (query.C_ID - 1) * query.O_OL_CNT + i + 1;// (query.W_ID - 1) * 3000 + query.C_ID;
      query.INFO[i].OL_SUPPLY_W_ID = w_id;
      query.INFO[i].OL_QUANTITY = 5;// random.uniform_dist(1, 10);
    }
    query.record_keys = keys;
    return query;
  }


  NewOrderQuery operator()(const std::vector<size_t>& keys, bool is_transmit) const {
    NewOrderQuery query;
    query.record_keys = keys;
    return query;
  }
  
};

struct PaymentQuery {
  int32_t W_ID;
  int32_t D_ID;
  int32_t C_ID;
  FixedString<16> C_LAST;
  int32_t C_D_ID;
  int32_t C_W_ID;
  float H_AMOUNT;

  std::vector<uint64_t> record_keys; // for migration
};

class makePaymentQuery {
public:
  PaymentQuery operator()(const Context &context, int32_t W_ID,
                          Random &random) const {
    PaymentQuery query;

    // W_ID is constant over the whole measurement interval

    query.W_ID = W_ID;

    // The district number (D_ID) is randomly selected within [1 ..10] from the
    // home warehouse (D_W_ID) = W_ID).

    query.D_ID = random.uniform_dist(1, 10);

    // the customer resident warehouse is the home warehouse 85% of the time
    // and is a randomly selected remote warehouse 15% of the time.

    // If the system is configured for a single warehouse,
    // then all customers are selected from that single home warehouse.

    int x = random.uniform_dist(1, 100);

    if (x <= context.paymentCrossPartitionProbability &&
        context.partition_num > 1) {
      // If x <= 15 a customer is selected from a random district number (C_D_ID
      // is randomly selected within [1 .. 10]), and a random remote warehouse
      // number (C_W_ID is randomly selected within the range of active
      // warehouses (see Clause 4.2.2), and C_W_ID ≠ W_ID).

      int32_t C_W_ID = W_ID;

      while (C_W_ID == W_ID) {
        C_W_ID = random.uniform_dist(1, context.partition_num);
      }

      query.C_W_ID = C_W_ID;
      query.C_D_ID = random.uniform_dist(1, 10);
    } else {
      // If x > 15 a customer is selected from the selected district number
      // (C_D_ID = D_ID) and the home warehouse number (C_W_ID = W_ID).

      query.C_D_ID = query.D_ID;
      query.C_W_ID = W_ID;
    }

    int y = random.uniform_dist(1, 100);

    // The customer is randomly selected 60% of the time by last name (C_W_ID ,
    // C_D_ID, C_LAST) and 40% of the time by number (C_W_ID , C_D_ID , C_ID).

    if (y <= 60) {
      // If y <= 60 a customer last name (C_LAST) is generated according to
      // Clause 4.3.2.3 from a non-uniform random value using the
      // NURand(255,0,999) function.

      std::string last_name =
          random.rand_last_name(random.non_uniform_distribution(255, 0, 999));
      query.C_LAST.assign(last_name);
      query.C_ID = 0;
    } else {
      // If y > 60 a non-uniform random customer number (C_ID) is selected using
      // the NURand(1023,1,3000) function.
      query.C_ID = random.non_uniform_distribution(1023, 1, 3000);
    }

    // The payment amount (H_AMOUNT) is randomly selected within [1.00 ..
    // 5,000.00].

    query.H_AMOUNT = random.uniform_dist(1, 5000);
    return query;
  }
};
} // namespace tpcc
} // namespace star
