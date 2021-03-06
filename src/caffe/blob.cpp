#include <climits>
#include <vector>

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/syncedmem.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {


template <typename Dtype>
void Blob<Dtype>::Reshape(const int num, const int channels, const int height, const int width) 
{
  num_ = num;
  channels_ = channels;
  height_ = height;
  width_ = width;
  count_ = num * channels * height * width;
  if (count_ > capacity_)
  {
    capacity_ = count_;
    data_.reset(new SyncedMemory(capacity_ * sizeof(Dtype)));
    diff_.reset(new SyncedMemory(capacity_ * sizeof(Dtype)));
    sec_diff_.reset(new SyncedMemory(capacity_ * sizeof(Dtype)));
  }
}

template <typename Dtype>
void Blob<Dtype>::set_data(Blob& other) 
{
	other.Reshape(num_,channels_,height_,width_);
  data_ = other.data();
}
template <typename Dtype>
void Blob<Dtype>::set_diff(Blob& other) 
{
  other.Reshape(num_,channels_,height_,width_);
  diff_ = other.diff();
}
template <typename Dtype>
void Blob<Dtype>::set_sec_diff(Blob& other) 
{
  other.Reshape(num_,channels_,height_,width_);
  sec_diff_ = other.sec_diff();
}
//----------------------------------------- proto <->  memory--------------------
template <typename Dtype>
void Blob<Dtype>::FromProto(const BlobProto& proto) 
{
  
  
  Dtype* data_vec = mutable_cpu_data();
  if (proto.data_size() > 0)
  {
    CHECK_EQ(count_, proto.data_size());
    for (int i = 0; i < count_; ++i)
      data_vec[i] = proto.data(i);
  }
  
  Dtype* diff_vec = mutable_cpu_diff();
  if (proto.diff_size() > 0)
  {
    CHECK_EQ(count_, proto.diff_size());
    
    for (int i = 0; i < count_; ++i)
      diff_vec[i] = proto.diff(i);
  }
}
template <typename Dtype>
void Blob<Dtype>::ToProto(BlobProto* proto, bool write_diff) const 
{
  
  proto->set_num(num_);
  proto->set_channels(channels_);
  proto->set_height(height_);
  proto->set_width(width_);
  
  
  proto->clear_data();
  proto->clear_diff();
  const Dtype* data_vec = cpu_data();
  for (int i = 0; i < count_; ++i)
    proto->add_data(data_vec[i]);
  
  if (write_diff)
  {
    const Dtype* diff_vec = cpu_diff();
    for (int i = 0; i < count_; ++i)
      proto->add_diff(diff_vec[i]);
  }
}
//-----------------------------------------------------------------------------------


INSTANTIATE_CLASS(Blob);
template class Blob<bool>;
template class Blob<int>;
template class Blob<unsigned int>;

}  // namespace caffe

