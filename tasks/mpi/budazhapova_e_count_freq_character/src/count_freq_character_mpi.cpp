﻿#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "mpi/budazhapova_e_count_freq_character/include/count_freq_character_mpi_header.hpp"

using namespace std::chrono_literals;

int budazhapova_e_count_freq_character_mpi::counting_freq(std::string str) {
  char symb = str[0];
  int res = 0;
  for (unsigned long i = 0; i < str.length(); i++) {
    if (str[i] == symb) {
      res++;
    }
  }
  return res;
}

std::string budazhapova_e_count_freq_character_mpi::getRandomString(int length) {
  static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
  std::string result;
  result.resize(length);

  srand(time(nullptr));
  for (int i = 0; i < length; i++) result[i] = charset[rand() % charset.length()];

  return result;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskSequential::pre_processing() {
  internal_order_test();
  input_ = std::string(reinterpret_cast<char*>(taskData->inputs[0]), taskData->inputs_count[0]);
  symb = input_[0];
  res = 0;
  return true;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskSequential::validation() {
  internal_order_test();
  res = counting_freq(input_);
  return taskData->outputs_count[0] == 1;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskSequential::run() {
  internal_order_test();
  symb = input_[0];
  for (unsigned long i = 0; i < input_.length(); i++) {
    if (input_[i] == symb) {
      res++;
    }
  }
  return true;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskSequential::post_processing() {
  internal_order_test();
  reinterpret_cast<int*>(taskData->outputs[0])[0] = res;
  return true;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskParallel::pre_processing() {
  internal_order_test();
  int world_rank = world.rank();
  unsigned int delta = 0;

  if (world_rank == 0) {
    delta = taskData->inputs_count[0] / world.size();
  }
  boost::mpi::broadcast(world, delta, 0);

  if (world_rank == 0) {
    input_ = std::string(reinterpret_cast<char*>(taskData->inputs[0]));
    auto* tmp_ptr = reinterpret_cast<char*>(taskData->inputs[0]);
    for (unsigned i = 0; i < taskData->inputs_count[0]; i++) {
      input_[i] = tmp_ptr[i];
    }
    symb = input_[0];
  }
  local_input_.resize(delta);
  if (world_rank == 0) {
    for (int proc = 1; proc < world.size(); proc++) {
      world.send(proc, 0, input_.data() + proc * delta, delta);
    }
    local_input_ = std::string(input_.begin(), input_.begin() + delta);
  } else {
    world.recv(0, 0, local_input_.data(), delta);
  }
  local_res = 0;
  return true;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskParallel::validation() {
  internal_order_test();
  if (world.rank() == 0) {
    return taskData->outputs_count[0] == 1;
  }
  return true;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskParallel::run() {
  internal_order_test();
  local_res = counting_freq(local_input_);
  reduce(world, local_res, res, std::plus(), 0);
  return true;
}

bool budazhapova_e_count_freq_character_mpi::TestMPITaskParallel::post_processing() {
  internal_order_test();
  if (world.rank() == 0) {
    reinterpret_cast<int*>(taskData->outputs[0])[0] = res;
  }
  return true;
}
