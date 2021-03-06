#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "caffe/layers/data/image_seg_data_layer.hpp"
#include "caffe/layer.hpp"
#include "caffe/util/benchmark.hpp"
#include "caffe/util/io.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/util/rng.hpp"
#include "caffe/solver.hpp"

namespace caffe {

template <typename Dtype>
void ImageSegDataLayer<Dtype>::DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
                                              const vector<Blob<Dtype>*>& top)
{
  TransformationParameter transform_param = this->layer_param_.transform_param();
  DataParameter image_data_param    = this->layer_param_.data_param();
  const int batch_size = image_data_param.batch_size();
  const int crop_size = transform_param.crop_size();

  // Read the file with filenames and labels
  const string& source = image_data_param.source(0);
  LOG(INFO) << "Opening file " << source;
  std::ifstream infile(source.c_str());
  string linestr;
  lines_.clear();
  while (std::getline(infile, linestr))
  {
    std::istringstream iss(linestr);
    string imgfn;
    iss >> imgfn;
    string segfn = imgfn;
    imgfn = "JPEGImages/" + imgfn + ".jpg";
    lines_.push_back(std::make_pair(imgfn, segfn));
  }

  if (image_data_param.shuffle()) {
    // randomly shuffle data
    LOG(INFO) << "Shuffling data";
    shuffle(lines_.begin(), lines_.end());
  }
  LOG(INFO) << "A total of " << lines_.size() << " images.";
  lines_id_ = 0;
	second_dataset = false;
	


  if(crop_size == 0)
  {
  	this->prefetch_data_.Reshape(1, 3, 224, 224);
  	this->prefetch_label_.Reshape(1, 1, 224, 224);
  }
  else
  {
	  this->prefetch_data_.Reshape(batch_size, 3, crop_size, crop_size);		  		  
	  this->prefetch_label_.Reshape(batch_size, 1, crop_size, crop_size);		 

		this->transformed_data_.Reshape(1, 3, crop_size, crop_size);
		this->transformed_label_.Reshape(1, 1, crop_size, crop_size);
  }
}
// This function is used to create a thread that prefetches the data.
template <typename Dtype>
void ImageSegDataLayer<Dtype>::InternalThreadEntry(int gpu_id)
{

 //new thread treat GPU 0 as default device, so it is necessary to set device in case of
  //ghost memory
#ifndef CPU_ONLY
  CUDA_CHECK(cudaSetDevice(Caffe::GPUs[gpu_id]));
#endif

  DataParameter image_data_param    = this->layer_param_.data_param();
  TransformationParameter transform_param = this->layer_param_.transform_param();
  const int ignore_label = transform_param.ignore_label();
  string root_folder   = image_data_param.root_folder();
  const bool random_scale = transform_param.random_scale();
  const bool random_aspect = transform_param.random_aspect();
  const bool random_rotate = transform_param.random_rotate();
  const int resolution = transform_param.resolution();
  int classes = transform_param.classes();
  int batch_size = image_data_param.batch_size();
  int crop_size = transform_param.crop_size();


	
	if (image_data_param.source_size() > 1 && Solver<Dtype>::iter() == 30000 && !second_dataset)
	{
		second_dataset = true;
		const string& source = image_data_param.source(1);
		LOG(INFO) << "Opening file " << source;
		std::ifstream infile(source.c_str());
		string linestr;
		lines_.clear();
		while (std::getline(infile, linestr))
		{
		  std::istringstream iss(linestr);
		  string imgfn;
		  iss >> imgfn;
		  string segfn = imgfn;
		  imgfn = "JPEGImages/" + imgfn + ".jpg";
		  lines_.push_back(std::make_pair(imgfn, segfn));
		}
		LOG(INFO) << "A total of " << lines_.size() << " images.";
  	lines_id_ = 0;
	}
	
			
	
	
  const int lines_size = lines_.size();

  for(int item_id=0;item_id < batch_size;item_id++)
  {

   
    std::vector<cv::Mat> cv_img_seg;
    cv_img_seg.clear();


    CHECK_GT(lines_size, lines_id_);

    int img_row, img_col;
    if(classes == 21)
    {
      if(lines_[lines_id_].second[0] != 't' && lines_[lines_id_].second[0] != 'v')
      {
        cv_img_seg.push_back(cv::imread(root_folder + "VOCdevkit/VOC2012/" + "JPEGImages/" + lines_[lines_id_].second + ".jpg", CV_LOAD_IMAGE_COLOR));
        if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
        cv_img_seg.push_back(cv::imread(root_folder + "VOCdevkit/VOC2012/" + "SegmentationClass/" + lines_[lines_id_].second + ".png", CV_LOAD_IMAGE_GRAYSCALE));
        if (!cv_img_seg[1].data) {LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png";}
      }
      else
      {
        cv_img_seg.push_back(cv::imread(root_folder + "MSCOCO/coco-master/" + "JPEGImages/" + lines_[lines_id_].second + ".jpg", CV_LOAD_IMAGE_COLOR));
        if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
        cv_img_seg.push_back(cv::imread(root_folder + "MSCOCO/coco-master/" + "SegmentationClass_reduced/" + lines_[lines_id_].second + ".png", CV_LOAD_IMAGE_GRAYSCALE));
        if (!cv_img_seg[1].data) {LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass_reduced/" + lines_[lines_id_].second + ".png";}
      }
    }
    else if(classes == 81)
    {
      cv_img_seg.push_back(cv::imread(root_folder + "JPEGImages/" + lines_[lines_id_].second + ".jpg", CV_LOAD_IMAGE_COLOR));
      if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
      cv_img_seg.push_back(cv::imread(root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png", CV_LOAD_IMAGE_GRAYSCALE));
      if (!cv_img_seg[1].data) {LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png";}
    }
    else if(classes == 60)
    {
      cv_img_seg.push_back(cv::imread(root_folder + "JPEGImages/" + lines_[lines_id_].second + ".jpg", CV_LOAD_IMAGE_COLOR));
      if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
      cv_img_seg.push_back(cv::imread(root_folder+"pascal_context/" + "SegmentationClass/" + lines_[lines_id_].second + ".png", CV_LOAD_IMAGE_GRAYSCALE));
      if (!cv_img_seg[1].data) {LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png";}
    }
    else if(classes == 19)
    {
      cv_img_seg.push_back(cv::imread(root_folder + "JPEGImages/" + lines_[lines_id_].second + "_leftImg8bit.png", CV_LOAD_IMAGE_COLOR));
      if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
			cv::Mat exist_gt = cv::imread(root_folder + "SegmentationClass/" + lines_[lines_id_].second + "_gtFine_labelIds.png", CV_LOAD_IMAGE_GRAYSCALE);
      if (!exist_gt.data) 
			{
				exist_gt = cv::imread(root_folder + "SegmentationClass/" + lines_[lines_id_].second + "_gtCoarse_labelIds.png", CV_LOAD_IMAGE_GRAYSCALE);
				if (!exist_gt.data) 
					LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png";
   		}
			cv_img_seg.push_back(exist_gt);
		}
    else if(classes == 150)
    {
      cv_img_seg.push_back(cv::imread(root_folder + "JPEGImages/" + lines_[lines_id_].second + ".jpg", CV_LOAD_IMAGE_COLOR));
      if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
      cv_img_seg.push_back(cv::imread(root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png", CV_LOAD_IMAGE_GRAYSCALE));
      if (!cv_img_seg[1].data) {LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png";}
    }
    else if(classes == 18)
    {
      cv_img_seg.push_back(cv::imread(root_folder + "JPEGImages/" + lines_[lines_id_].second + ".jpg", CV_LOAD_IMAGE_COLOR));
      if (!cv_img_seg[0].data) {LOG(FATAL) << "Fail to load img: " << root_folder + lines_[lines_id_].first;}
      cv_img_seg.push_back(cv::imread(root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png", CV_LOAD_IMAGE_GRAYSCALE));
      if (!cv_img_seg[1].data) {LOG(FATAL) << "Fail to load seg: " << root_folder + "SegmentationClass/" + lines_[lines_id_].second + ".png";}
    }
    else
    {
      LOG(FATAL)<<"wrong classes";
    }

  	
  	if(random_rotate)
  	{
  		int angle = caffe_rng_rand()%30 - 15;
      //cv::imwrite("raw_image.jpg",cv_img_seg[0]);
      //cv::imwrite("raw_label.jpg",cv_img_seg[1]);    
  		cv::Mat M = cv::getRotationMatrix2D(cv::Point2f(cv_img_seg[0].cols/2,cv_img_seg[0].rows/2), float(angle), float(1.0) );
  	
  		unsigned int b= 0;
			unsigned int g= 0;
			unsigned int r= 0;
  		cv::warpAffine(cv_img_seg[0], cv_img_seg[0],
		                          M, cv::Size(cv_img_seg[0].cols,cv_img_seg[0].rows),
		                          CV_INTER_CUBIC,
		                          cv::BORDER_CONSTANT,
		                          cv::Scalar(b,g,r));
  
			cv::warpAffine(cv_img_seg[1], cv_img_seg[1],
		                          M, cv::Size(cv_img_seg[1].cols,cv_img_seg[1].rows),
		                          CV_INTER_NN,
		                          cv::BORDER_CONSTANT,
		                          cv::Scalar(ignore_label));			
			//cv::imwrite("debug_image.jpg",cv_img_seg[0]);
			//cv::imwrite("debug_label.png",cv_img_seg[1]);
			//LOG(FATAL)<<"-------------------------";                
		}

    if(random_scale)
    {
      int temp = caffe_rng_rand()%200 - 100;
      //LOG(INFO)<<"temp = "<<temp;
      Dtype ratio = std::pow(2,Dtype(temp)/100);

      int width = ratio * cv_img_seg[0].cols;
      int height = ratio * cv_img_seg[0].rows;

      if(random_aspect)
      {
        temp = caffe_rng_rand()%40 + 80;
        Dtype aspect = Dtype(temp) / 100;

        width = ratio * aspect * cv_img_seg[0].cols;
        height = ratio *aspect * cv_img_seg[0].rows;
      }

      cv::resize(cv_img_seg[0],cv_img_seg[0],cv::Size(width,height),0,0,CV_INTER_CUBIC);
      cv::resize(cv_img_seg[1],cv_img_seg[1],cv::Size(width,height),0,0,CV_INTER_NN);
    }

    int width=cv_img_seg[0].cols;
    int height=cv_img_seg[0].rows;
    if(transform_param.random_scale())
    {
      if(width < 224 || height < 224)
      {
        if(width < height)
        {
          height = 224 / Dtype(width) * Dtype(height);
          width = 224;
          cv::resize(cv_img_seg[0],cv_img_seg[0],cv::Size(width,height),0,0,CV_INTER_CUBIC);
          cv::resize(cv_img_seg[1],cv_img_seg[1],cv::Size(width,height),0,0,CV_INTER_NN);
        }
        else
        {
          width = 224 / Dtype(height) * Dtype(width);
          height = 224;
          cv::resize(cv_img_seg[0],cv_img_seg[0],cv::Size(width,height),0,0,CV_INTER_CUBIC);
          cv::resize(cv_img_seg[1],cv_img_seg[1],cv::Size(width,height),0,0,CV_INTER_NN);
        }
      }    
    }	

	
    
    if(crop_size == 0)
    {
      width=cv_img_seg[0].cols;
      height=cv_img_seg[0].rows;
      int pad_width = (resolution - width  % resolution) % resolution;
      int pad_height = (resolution - height % resolution) % resolution;
      this->prefetch_data_.Reshape(1, 3, height+pad_height, width+pad_width);
      this->prefetch_label_.Reshape(1, 1, height+pad_height, width+pad_width);
      this->data_transformer_->TransformImgAndSeg(cv_img_seg, &(this->prefetch_data_), &(this->prefetch_label_), ignore_label);
    }
    else
    {
      this->transformed_data_.set_cpu_data(this->prefetch_data_.mutable_cpu_data()+
      														this->prefetch_data_.offset(item_id));
      this->transformed_label_.set_cpu_data(this->prefetch_label_.mutable_cpu_data()+
      														this->prefetch_label_.offset(item_id));
      this->data_transformer_->TransformImgAndSeg(cv_img_seg, &(this->transformed_data_), &(this->transformed_label_), ignore_label);    
    }


    lines_id_++;
    if (lines_id_ >= lines_size)
    {
      lines_id_ = 0;
      if (image_data_param.shuffle())
        shuffle(lines_.begin(), lines_.end());
    }
  }
}

template <typename Dtype>
ImageSegDataLayer<Dtype>::~ImageSegDataLayer<Dtype>() 
{
  if (this->thread_.get() != NULL && this->thread_->joinable())
    this->thread_->join();
}

INSTANTIATE_CLASS(ImageSegDataLayer);
REGISTER_LAYER_CLASS(ImageSegData);
}  // namespace caffe
