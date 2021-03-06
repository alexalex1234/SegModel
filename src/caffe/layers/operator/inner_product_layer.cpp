#include <vector>

#include "caffe/filler.hpp"
#include "caffe/layers/operator/inner_product_layer.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template <typename Dtype>
void InnerProductLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) 
{
  int num = bottom[0]->num();
  int channels = bottom[0]->channels();
  int height = bottom[0]->height();
  int width = bottom[0]->width();
  
  num_output = this->layer_param_.inner_product_param().num_output();



  if (this->blobs_.size() > 0)
    LOG(INFO) << "Skipping parameter initialization";
  else
  {
    if (this->layer_param_.inner_product_param().bias_term())
      this->blobs_.resize(2);
    else
      this->blobs_.resize(1);
    
    this->blobs_[0].reset(new Blob<Dtype>(num_output,channels*height*width,1,1));
    shared_ptr<Filler<Dtype> > weight_filler(GetFiller<Dtype>(this->layer_param_.inner_product_param().weight_filler()));
    weight_filler->Fill(this->blobs_[0].get());

    if (this->lr_mult().size() == 0)
    {
    	this->lr_mult().push_back(1);
    	this->decay_mult().push_back(1);
    }	
    if (this->layer_param_.inner_product_param().bias_term())
    {
      this->blobs_[1].reset(new Blob<Dtype>(num_output,1,1,1));
      caffe_set(this->blobs_[1]->count(),Dtype(0),this->blobs_[1]->mutable_cpu_data());

      if (this->lr_mult().size() == 1)
    	{
		  	this->lr_mult().push_back(2);
		  	this->decay_mult().push_back(0);
    	}
    }
  }
  
  if (this->layer_param_.inner_product_param().bias_term())
  {
    bias_multiplier_.Reshape(num,1,1,1);
    caffe_set(num, Dtype(1), bias_multiplier_.mutable_cpu_data());
  }
}

template <typename Dtype>
void InnerProductLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) 
{
  
  int num = bottom[0]->num();
  
  top[0]->Reshape(num,num_output,1,1);
}


INSTANTIATE_CLASS(InnerProductLayer);
REGISTER_LAYER_CLASS(InnerProduct);

}  // namespace caffe
