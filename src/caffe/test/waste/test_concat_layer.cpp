
	#include <cstring>
#include <vector>

#include "gtest/gtest.h"

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/filler.hpp"
#include "caffe/layers/func/concat_layer.hpp"

#include "caffe/test/test_gradient_check_util.hpp"

namespace caffe {

template <typename TypeParam>
class ConcatLayerTest : public ::testing::Test
{
  typedef TypeParam Dtype;
 protected:
  ConcatLayerTest() : blob_bottom_0_(new Blob<Dtype>()), blob_bottom_1_(new Blob<Dtype>()),blob_top_(new Blob<Dtype>())
  {
  	caffe::Caffe::parallel_workspace_.resize(48);
		for (int i=0;i<12;i++)
			caffe::Caffe::parallel_workspace_[i] = new caffe::Blob<TypeParam>();
  }
  virtual void SetUp()
  {
    blob_bottom_0_->Reshape(2, 3, 4, 3);
		blob_bottom_1_->Reshape(2, 4, 4, 3);
    caffe_rng_gaussian<Dtype>(this->blob_bottom_0_->count(), Dtype(0), Dtype(1), 
    													this->blob_bottom_0_->mutable_cpu_data());
		caffe_rng_gaussian<Dtype>(this->blob_bottom_1_->count(), Dtype(0), Dtype(1), 
    													this->blob_bottom_1_->mutable_cpu_data());					
    													
    blob_bottom_vec_.push_back(blob_bottom_0_);
    blob_bottom_vec_.push_back(blob_bottom_1_);
    blob_top_vec_.push_back(blob_top_);
  }
  virtual ~ConcatLayerTest()
  {
    delete blob_bottom_0_;
    delete blob_bottom_1_;
    delete blob_top_;
  }
  Blob<Dtype>* const blob_bottom_0_;
  Blob<Dtype>* const blob_bottom_1_;
  Blob<Dtype>* const blob_top_;
  vector<Blob<Dtype>*> blob_bottom_vec_;
  vector<Blob<Dtype>*> blob_top_vec_;
};

typedef testing::Types<float> myTypes;

TYPED_TEST_CASE(ConcatLayerTest, myTypes);


TYPED_TEST(ConcatLayerTest, TestGradient)
{
  typedef TypeParam Dtype;

  LayerParameter layer_param;
  ConcatLayer<Dtype> layer(layer_param);
  layer.SetUp(this->blob_bottom_vec_, this->blob_top_vec_);
  layer.Reshape(this->blob_bottom_vec_, this->blob_top_vec_);
  layer.Forward(this->blob_bottom_vec_, this->blob_top_vec_);
  
  GradientChecker<Dtype> checker(1e-2, 1e-2);
  checker.CheckGradientExhaustive(&layer, this->blob_bottom_vec_, this->blob_top_vec_);
	checker.CheckSecGradientExhaustive(&layer,  this->blob_top_vec_, this->blob_bottom_vec_);
}


}  // namespace caffe
