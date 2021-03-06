
#include <algorithm>
#include <vector>

#include "caffe/layers/operator/cudnn_deconv_layer.hpp"

namespace caffe {

#define CUDNN_STREAMS 1

template <typename Dtype>
void CuDNNDeConvolutionLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) 
{
	CUDA_CHECK(cudaGetDevice(&gpu_id_));
	int i;
	for (i=0;i<Caffe::GPUs.size();i++)
		if (Caffe::GPUs[i] == gpu_id_)
			break;
	gpu_id_ = i;

  DeConvolutionLayer<Dtype>::LayerSetUp(bottom, top);
	
//----------------------------------------	
	myworkspace_.resize(1);
	myworkspace_[0] = static_cast<Blob<Dtype> *>(Caffe::parallel_workspace_[gpu_id_]);
//----------------------------------------	


  // Create filter descriptor.

	
	cudnn::createFilterDesc<Dtype>(&filter_desc_,
	    this->channels_/this->group_ , this->num_output_/this->group_, this->kernel_size_, this->kernel_size_); 
	// Tensor descriptor for bias.
  if (this->layer_param_.convolution_param().bias_term()) 
  {
    cudnn::createTensor4dDesc<Dtype>(&bias_desc_);
   	cudnn::setTensor4dDesc<Dtype>(&bias_desc_,
      1, this->num_output_/this->group_, 1, 1); 
  } 


  cudnn::createTensor4dDesc<Dtype>(&bottom_descs_);
  cudnn::createTensor4dDesc<Dtype>(&top_descs_);
  cudnn::createConvolutionDesc<Dtype>(&conv_descs_);      
}

template <typename Dtype>
void CuDNNDeConvolutionLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) 
{
  DeConvolutionLayer<Dtype>::Reshape(bottom, top);

  const int num = bottom[0]->num();
  const int height = bottom[0]->height();
  const int width = bottom[0]->width();
  const int height_out = top[0]->height();
  const int width_out = top[0]->width();


  if (this->group_ == 1)
 	{
		cudnn::setTensor4dDesc<Dtype>(&bottom_descs_,
		    num, this->channels_ , height, width);
		cudnn::setTensor4dDesc<Dtype>(&top_descs_,
		    num, this->num_output_ , height_out, width_out);  
	}
	else
	{
		CHECK_EQ(this->channels_,this->num_output_);
		cudnn::setTensor4dDesc<Dtype>(&bottom_descs_,
		    1, this->channels_/this->group_ , height, width);
		cudnn::setTensor4dDesc<Dtype>(&top_descs_,
		    1, this->num_output_/this->group_ , height_out, width_out);  
	}
	cudnn::setConvolutionDesc<Dtype>(&conv_descs_, 
		    this->pad_, this->pad_, this->stride_, this->stride_, this->filter_stride_, this->filter_stride_);
		    

	size_t workspace_limit_bytes = 511888000;
	if (num == 1)  workspace_limit_bytes = 0;
		
	CUDNN_CHECK(cudnnGetConvolutionForwardAlgorithm(Caffe::cudnn_handle(gpu_id_),
					top_descs_,
					filter_desc_,
					conv_descs_,
					bottom_descs_,
					CUDNN_CONVOLUTION_FWD_SPECIFY_WORKSPACE_LIMIT,
					workspace_limit_bytes,
					&fwd_algo_));			
	CUDNN_CHECK(cudnnGetConvolutionBackwardFilterAlgorithm(Caffe::cudnn_handle(gpu_id_),
		  top_descs_, 
		  bottom_descs_, 
		  conv_descs_, 
		  filter_desc_,
		  CUDNN_CONVOLUTION_BWD_FILTER_SPECIFY_WORKSPACE_LIMIT,
		  workspace_limit_bytes, 
		  &bwd_filter_algo_) );
	CUDNN_CHECK(cudnnGetConvolutionBackwardDataAlgorithm(Caffe::cudnn_handle(gpu_id_),
		  filter_desc_, 
		  bottom_descs_, 
		  conv_descs_, 
		  top_descs_,
		  CUDNN_CONVOLUTION_BWD_DATA_SPECIFY_WORKSPACE_LIMIT,
		  workspace_limit_bytes, 
		  &bwd_data_algo_)); 
	
	CUDNN_CHECK(cudnnGetConvolutionForwardWorkspaceSize(Caffe::cudnn_handle(gpu_id_),
		top_descs_,
		filter_desc_,
		conv_descs_,
		bottom_descs_,
		fwd_algo_,
		&(workspace_fwd_sizes_)));			   
	CUDNN_CHECK(cudnnGetConvolutionBackwardFilterWorkspaceSize(Caffe::cudnn_handle(gpu_id_),
			top_descs_, 
			bottom_descs_, 
			conv_descs_, 
			filter_desc_,
			bwd_filter_algo_, 
			&workspace_bwd_filter_sizes_));    
	CUDNN_CHECK(cudnnGetConvolutionBackwardDataWorkspaceSize(Caffe::cudnn_handle(gpu_id_),
			filter_desc_, 
			bottom_descs_,
			conv_descs_, 
			top_descs_, 
			bwd_data_algo_, 
			&workspace_bwd_data_sizes_) );   
//-----------------------------------------------------------------------------------------	
		myworkspace_[0]->Reshape(workspace_fwd_sizes_/sizeof(Dtype)+1,1,1,1);
	 	myworkspace_[0]->Reshape(workspace_bwd_data_sizes_/sizeof(Dtype)+1,1,1,1);
	 	myworkspace_[0]->Reshape(workspace_bwd_filter_sizes_/sizeof(Dtype)+1,1,1,1);    
//-----------------------------------------------------------------------------------------	     
}

template <typename Dtype>
CuDNNDeConvolutionLayer<Dtype>::~CuDNNDeConvolutionLayer() 
{
  cudnnDestroyTensorDescriptor(bottom_descs_);
  cudnnDestroyTensorDescriptor(top_descs_);
  cudnnDestroyConvolutionDescriptor(conv_descs_);
  
  if (this->layer_param_.convolution_param().bias_term()) 
    cudnnDestroyTensorDescriptor(bias_desc_);

  cudnnDestroyFilterDescriptor(filter_desc_);
}

INSTANTIATE_CLASS(CuDNNDeConvolutionLayer);
REGISTER_LAYER_CLASS(CuDNNDeConvolution);
}   // namespace caffe
