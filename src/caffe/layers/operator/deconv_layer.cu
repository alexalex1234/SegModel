// Copyright 2013 Yangqing Jia

#include <vector>

#include "caffe/layer.hpp"
#include "caffe/layers/operator/deconv_layer.hpp"
#include "caffe/util/im2col.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template <typename Dtype>
void DeConvolutionLayer<Dtype>::Forward_gpu(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*> &top) 
{
  const Dtype* bottom_data = bottom[0]->gpu_data();
  Dtype* top_data = top[0]->mutable_gpu_data();
  Dtype* col_data = col_buffer_->mutable_gpu_data();
  const Dtype* weight = this->blobs_[0]->gpu_data();
  
  int num = bottom[0]->num();
  int channels = bottom[0]->channels();
  int height = bottom[0]->height();
  int width = bottom[0]->width();
  
 	int bottom_offset_ = height * width * channels / group_;
 	int col_offset_ = height * width * kernel_size_ * kernel_size_ * num_output_ / group_;
 	int weight_offset_ = kernel_size_ * kernel_size_ * channels * num_output_ / group_ / group_;
 	

  for (int n = 0; n < num; n++) 
  {
  	for (int g = 0; g < group_; g++) 
  	{
		  caffe_gpu_gemm<Dtype>(CblasTrans, CblasNoTrans, kernel_size_*kernel_size_*num_output_/group_, height*width,  channels/group_,
														(Dtype)1., weight + weight_offset_ * g, bottom_data + bottom[0]->offset(n) + bottom_offset_ * g,
														(Dtype)0., col_data + col_offset_ * g);
  	}

 		
    col2im_gpu(col_data, num_output_, height_out_, width_out_,  
    kernel_size_, kernel_size_, pad_, pad_, stride_, stride_, filter_stride_, filter_stride_, 
    top_data + top[0]->offset(n));

   
  
    if (this->layer_param_.convolution_param().bias_term()) 
    {
      caffe_gpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, num_output_,height_out_*width_out_, 1, 
													(Dtype)1., this->blobs_[1]->gpu_data(), bias_multiplier_->gpu_data(),
													(Dtype)1., top_data + top[0]->offset(n));
    }      
  }     
}

template <typename Dtype>
void DeConvolutionLayer<Dtype>::Backward_gpu(const vector<Blob<Dtype>*>& top, const vector<Blob<Dtype>*>& bottom) 
{
	int num = bottom[0]->num();
  int channels = bottom[0]->channels();
  int height = bottom[0]->height();
  int width = bottom[0]->width();
//-------------------------------------------------------------------------
	if (this->layer_param_.convolution_param().bias_term())
  {
    bias_multiplier_->Reshape(1,1,height_out_,width_out_);
    caffe_gpu_set(bias_multiplier_->count(),Dtype(1),bias_multiplier_->mutable_gpu_data());  
  }
  col_buffer_->Reshape(kernel_size_*kernel_size_*num_output_, height*width, 1, 1);
//-------------------------------------------------------------------------

  const Dtype* top_diff = top[0]->gpu_diff();
  const Dtype* weight = this->blobs_[0]->gpu_data();
  const Dtype* bottom_data = bottom[0]->gpu_data();
  
  Dtype* bottom_diff = bottom[0]->mutable_gpu_diff();
  Dtype* weight_diff = this->blobs_[0]->mutable_gpu_diff();
  Dtype* col_data = col_buffer_->mutable_gpu_data();
  Dtype* col_diff = col_buffer_->mutable_gpu_diff();




	int bottom_offset_ = height * width * channels / group_;
 	int col_offset_ = height * width * kernel_size_ * kernel_size_ * num_output_ / group_;
 	int weight_offset_ = kernel_size_ * kernel_size_ * channels * num_output_ / group_ /group_;
	

	if (this->layer_param_.convolution_param().bias_term() && this->lr_mult()[1] != 0) 
	{
		Dtype* bias_diff = this->blobs_[1]->mutable_gpu_diff();
		for (int n = 0; n < num; ++n)  
		{
			caffe_gpu_gemv<Dtype>(CblasNoTrans, num_output_, height_out_ * width_out_, 
														(Dtype)1., top_diff + top[0]->offset(n), bias_multiplier_->gpu_data(), 
														(Dtype)1., bias_diff);
		}
	}
	
	if (this->lr_mult()[0] != 0)
	{
		for (int n = 0; n < num; ++n) 
		{
			im2col_gpu(top_diff + top[0]->offset(n), num_output_, height_out_,width_out_, 
		  kernel_size_, kernel_size_, pad_, pad_, stride_, stride_, filter_stride_, filter_stride_, 
		  col_diff); 
		
			for (int g = 0; g < group_; ++g) 
			{
				caffe_gpu_gemm<Dtype>(CblasNoTrans, CblasTrans, channels/group_,  kernel_size_*kernel_size_*num_output_/group_, height*width,
															(Dtype)1., bottom_data + bottom[0]->offset(n) + bottom_offset_ * g, col_diff + col_offset_ * g, 
															(Dtype)1., weight_diff + weight_offset_ * g);
			}												
		}
	}
  for (int n = 0; n < num; ++n) 
  {
  	
    im2col_gpu(top_diff + top[0]->offset(n), num_output_, height_out_,width_out_, 
    kernel_size_, kernel_size_, pad_, pad_, stride_, stride_, filter_stride_, filter_stride_, 
    col_diff);   
    
    for (int g = 0; g < group_; ++g) 
  	{
		  caffe_gpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, channels/group_, height*width, kernel_size_*kernel_size_*num_output_/group_,
														(Dtype)1., weight + weight_offset_ * g, col_diff + col_offset_ * g,
														(Dtype)0., bottom_diff + bottom[0]->offset(n) + bottom_offset_ * g);
		}												
  }    
}
template <typename Dtype>
void DeConvolutionLayer<Dtype>::SecForward_gpu(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*> &top) 
{
}
INSTANTIATE_LAYER_GPU_FUNCS(DeConvolutionLayer);

}  // namespace caffe
