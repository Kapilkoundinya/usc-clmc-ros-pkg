/*********************************************************************
 Computational Learning and Motor Control Lab
 University of Southern California
 Prof. Stefan Schaal
 *********************************************************************
 \remarks   ...

 \file    task_recorder_io.h

 \author  Peter Pastor, Mrinal Kalakrishnan
 \date    Jul 14, 2010

 *********************************************************************/

#ifndef TASK_RECORDER_IO_H_
#define TASK_RECORDER_IO_H_

// system includes

// ros includes
#include <ros/ros.h>
#include <ros/package.h>
#include <rosbag/bag.h>

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>

#include <usc_utilities/assert.h>
#include <usc_utilities/param_server.h>
#include <usc_utilities/file_io.h>

#include <task_recorder2_msgs/DataSample.h>
#include <task_recorder2_msgs/DataSampleLabel.h>

#include <task_recorder2_msgs/Description.h>
#include <task_recorder2_msgs/AccumulatedTrialStatistics.h>
#include <task_recorder2_utilities/task_recorder_utilities.h>
#include <task_recorder2_utilities/task_description_utilities.h>

#include <dmp_lib/trajectory.h>

// local includes

namespace task_recorder2_io
{

static const std::string RESAMPLED_DIRECTORY_NAME = "resampled";
static const std::string RAW_DIRECTORY_NAME = "raw";

// default template parameters
template<class MessageType = task_recorder2_msgs::DataSample> class TaskRecorderIO;

template<class MessageType>
  class TaskRecorderIO
  {

  public:

  typedef boost::shared_ptr<MessageType const> MessageTypeConstPtr;

    /*! Constructor
     */
    TaskRecorderIO(ros::NodeHandle node_handle) :
      node_handle_(node_handle),
      write_out_raw_data_(false), write_out_clmc_data_(false),
      write_out_resampled_data_(false), // write_out_statistics_(false),
      initialized_(false)
    {
      ROS_DEBUG("Reserving memory for >%i< messages.", NUMBER_OF_INITIALLY_RESERVED_MESSAGES);
      messages_.reserve(NUMBER_OF_INITIALLY_RESERVED_MESSAGES);
    };

    /*! Destructor
     */
    virtual ~TaskRecorderIO() {};

    /*!
     * @param topic_name
     * @param prefix
     * @return True on success, otherwise False
     */
    bool initialize(const std::string& topic_name, const std::string prefix = "");

    /*!
     * @param description
     */
    void setDescription(const task_recorder2_msgs::Description& description);

    /*!
     * @param description
     * @param directory_name
     * @return True on success, otherwise False
     */
    bool createDirectories(const std::string directory_name = std::string(""));
    bool createResampledDirectories()
    {
      return createDirectories(RESAMPLED_DIRECTORY_NAME);
    }
    bool createRawDirectories()
    {
      return createDirectories(RAW_DIRECTORY_NAME);
    }

    /*!
     * @return Last description that has been set
     */
    task_recorder2_msgs::Description getDescription() const;

    /*!
     * @param directory_name
     * @return True on success, otherwise False
     */
    bool writeRecordedData(const std::string& directory_name);
    bool writeRecordedDataSamples()
    {
      return writeRecordedData("");
    }
    bool writeResampledData()
    {
      return writeRecordedData(RESAMPLED_DIRECTORY_NAME);
    }
    bool writeRawData()
    {
      return writeRecordedData(RAW_DIRECTORY_NAME);
    }

    /*!
     * @param directory_name
     * @return True on success, otherwise False
     */
    bool incrementCounterFile(const std::string directory_name);
    bool incrementDataSamplesCounterFilt()
    {
      return incrementCounterFile("");
    }
    bool incrementResampledDataCounterFilt()
    {
      return incrementCounterFile(RESAMPLED_DIRECTORY_NAME);
    }
    bool incrementRawDataCounterFilt()
    {
      return incrementCounterFile(RAW_DIRECTORY_NAME);
    }

    /*!
     * @param directory_name
     * @return True on success, otherwise False
     */
    bool writeRecordedDataToCLMCFile(const std::string directory_name = std::string(""));

    /*!
     * @return True on success, otherwise False
     */
    // bool writeStatistics(std::vector<std::vector<task_recorder2_msgs::AccumulatedTrialStatistics> >& vector_of_accumulated_trial_statistics);

    /*!
     * @param description
     * @param abs_file_name
     * @return True on success, otherwise False
     */
    bool getAbsFileName(const task_recorder2_msgs::Description& description,
                        std::string& abs_file_name);


    /*!
     * @param description
     * @param data_samples
     * @return True on success, otherwise False
     */
    bool readDataSamples(const task_recorder2_msgs::Description& description,
                         std::vector<MessageType>& msgs);

    /*!
     * @param descriptions
     * @return True on success, otherwise False
     */
    bool getList(std::vector<std::string>& descriptions);

    /*!
     */
    ros::NodeHandle node_handle_;
    std::string topic_name_;
    std::string prefixed_topic_name_;

    /*!
     */
    std::vector<MessageType> messages_;
    bool write_out_raw_data_;
    bool write_out_clmc_data_;
    bool write_out_resampled_data_;
    // bool write_out_statistics_;

  private:

    static const int NUMBER_OF_INITIALLY_RESERVED_MESSAGES = 20 * 300;

    /*!
     */
    bool initialized_;

    /*!
     */
    task_recorder2_msgs::Description description_;
    std::string data_directory_name_;

    /*!
     */
    boost::filesystem::path absolute_data_directory_path_;

  };

template<class MessageType>
  bool TaskRecorderIO<MessageType>::initialize(const std::string& topic_name,
                                               const std::string prefix)
  {
    topic_name_ = topic_name;
    prefixed_topic_name_ = topic_name;
    usc_utilities::removeLeadingSlash(prefixed_topic_name_);
    prefixed_topic_name_ = prefix + prefixed_topic_name_;
    usc_utilities::appendLeadingSlash(prefixed_topic_name_);

    ROS_INFO("Initializing task recorder >%s< for topic named >%s<.", prefixed_topic_name_.c_str(), topic_name_.c_str());

    ROS_VERIFY(usc_utilities::read(node_handle_, "write_out_resampled_data", write_out_resampled_data_));
    ROS_VERIFY(usc_utilities::read(node_handle_, "write_out_raw_data", write_out_raw_data_));
    ROS_VERIFY(usc_utilities::read(node_handle_, "write_out_clmc_data", write_out_clmc_data_));
    // ROS_VERIFY(usc_utilities::read(node_handle_, "write_out_statistics", write_out_statistics_));

    std::string recorder_package_name;
    ROS_VERIFY(usc_utilities::read(node_handle_, "recorder_package_name", recorder_package_name));
    std::string recorder_data_directory_name;
    ROS_VERIFY(usc_utilities::read(node_handle_, "recorder_data_directory_name", recorder_data_directory_name));
    data_directory_name_ = task_recorder2_utilities::getDirectoryPath(recorder_package_name, recorder_data_directory_name);
    ROS_VERIFY(task_recorder2_utilities::checkAndCreateDirectories(data_directory_name_));
    ROS_DEBUG("Setting TaskRecorderIO data directory name to >%s<.", data_directory_name_.c_str());

    return (initialized_ = true);
  }

template<class MessageType>
  void TaskRecorderIO<MessageType>::setDescription(const task_recorder2_msgs::Description& description)
  {
    ROS_ASSERT_MSG(initialized_, "Task recorder IO module is not initialized.");
    description_ = description;
  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::createDirectories(const std::string directory_name)
  {
    if (!task_recorder2_utilities::checkAndCreateDirectories(data_directory_name_))
    {
      ROS_ERROR("Could not create recorder_data directory >%s<."
          "This should never happen.", data_directory_name_.c_str());
      return false;
    }
    // check whether directory exists, if not, create it
    absolute_data_directory_path_ = boost::filesystem::path(data_directory_name_ + task_recorder2_utilities::getFileName(description_));
    // create "base" directory if it does not already exist
    if(!boost::filesystem::exists(absolute_data_directory_path_))
    {
      ROS_VERIFY_MSG(boost::filesystem::create_directory(absolute_data_directory_path_),
                     "Could not create directory >%s< : %s.",
                     absolute_data_directory_path_.directory_string().c_str(), std::strerror(errno));
      task_recorder2_utilities::createSymlinks(absolute_data_directory_path_);
    }
    boost::filesystem::path path = absolute_data_directory_path_;
    if(!directory_name.empty())
    {
      path = boost::filesystem::path(absolute_data_directory_path_.directory_string() + std::string("/") + directory_name);
    }
    if (!task_recorder2_utilities::checkForDirectory(path))
      return false;
    if (!task_recorder2_utilities::getTrialId(path, description_.trial, prefixed_topic_name_))
      return false;
    if (!task_recorder2_utilities::checkForCompleteness(path, description_.trial, prefixed_topic_name_))
      return false;
    ROS_DEBUG("Setting trial to >%i<.", description_.trial);
    return true;
  }

template<class MessageType>
  task_recorder2_msgs::Description TaskRecorderIO<MessageType>::getDescription() const
  {
    ROS_ASSERT_MSG(initialized_, "Task recorder IO module is not initialized.");
    return description_;
  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::incrementCounterFile(const std::string directory_name)
  {
    ROS_ASSERT_MSG(initialized_, "Task recorder IO module is not initialized.");
    boost::filesystem::path path = absolute_data_directory_path_;
    if (!directory_name.empty())
      path = boost::filesystem::path(absolute_data_directory_path_.directory_string() + std::string("/") + directory_name);

    if (!task_recorder2_utilities::incrementTrialCounterFile(path, prefixed_topic_name_))
      return false;
    if (!task_recorder2_utilities::getTrialId(path, description_.trial, prefixed_topic_name_))
      return false;
    if (!task_recorder2_utilities::checkForCompleteness(path, description_.trial, prefixed_topic_name_))
      return false;
    return true;
  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::writeRecordedData(const std::string& directory_name)
  {
    ROS_ASSERT_MSG(initialized_, "Task recorder IO module is not initialized.");
    ROS_VERIFY_MSG(task_recorder2_utilities::checkAndCreateDirectories(data_directory_name_),
                   "Could not create recorder_data directory >%s<. This should never happen.", data_directory_name_.c_str());
    std::string file_name = task_recorder2_utilities::getPathNameIncludingTrailingSlash(absolute_data_directory_path_);
    boost::filesystem::path path = absolute_data_directory_path_;
    if (!directory_name.empty())
    {
      file_name.append(directory_name);
      path = boost::filesystem::path(absolute_data_directory_path_.directory_string() + std::string("/") + directory_name);
      if (!task_recorder2_utilities::checkForDirectory(file_name))
        return false;
      usc_utilities::appendTrailingSlash(file_name);
    }
    if (!task_recorder2_utilities::getTrialId(path, description_.trial, prefixed_topic_name_))
      return false;
    file_name.append(task_recorder2_utilities::getDataFileName(prefixed_topic_name_, description_.trial));
    if (!usc_utilities::FileIO<MessageType>::writeToBagFileWithTimeStamps(messages_, topic_name_, file_name, false))
      return false;
    return incrementCounterFile(directory_name);
  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::getAbsFileName(const task_recorder2_msgs::Description& description,
                                                   std::string& abs_file_name)
  {
    boost::filesystem::path path = boost::filesystem::path(data_directory_name_ + task_recorder2_utilities::getFileName(description));
    abs_file_name = task_recorder2_utilities::getPathNameIncludingTrailingSlash(path);
    abs_file_name.append(task_recorder2_utilities::getDataFileName(prefixed_topic_name_, description.trial));
    return true;
  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::readDataSamples(const task_recorder2_msgs::Description& description,
                                                    std::vector<MessageType>& msgs)
  {
    std::string abs_file_name;
    if (!getAbsFileName(description, abs_file_name))
    {
      ROS_ERROR("Could not get file name of >%s<.", task_recorder2_utilities::getFileName(description).c_str());
      return false;
    }
    if (!usc_utilities::FileIO<MessageType>::readFromBagFile(msgs, topic_name_, abs_file_name, false))
    {
      ROS_ERROR("Could not read data samples in >%s<.", abs_file_name.c_str());
      return false;
    }
    return true;
  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::writeRecordedDataToCLMCFile(const std::string directory_name)
  {
    ROS_ASSERT_MSG(initialized_, "Task recorder IO module is not initialized.");
    ROS_ASSERT_MSG(!messages_.empty(), "Messages are empty. Cannot write anything to CLMC file.");

    if (!task_recorder2_utilities::checkAndCreateDirectories(data_directory_name_))
    {
      ROS_ERROR("Could not create recorder_data directory >%s<. This should never happen.", data_directory_name_.c_str());
      return false;
    }
    std::string file_name = task_recorder2_utilities::getPathNameIncludingTrailingSlash(absolute_data_directory_path_);
    boost::filesystem::path path = absolute_data_directory_path_;
    if (!directory_name.empty())
    {
      file_name.append(directory_name);
      path = boost::filesystem::path(absolute_data_directory_path_.directory_string() + std::string("/") + directory_name);
      if (!task_recorder2_utilities::checkForDirectory(file_name))
        return false;
      usc_utilities::appendTrailingSlash(file_name);
    }
    if (!task_recorder2_utilities::getTrialId(path, description_.trial, prefixed_topic_name_))
      return false;
    std::string clmc_file_name;
    if (!task_recorder2_utilities::setCLMCFileName(clmc_file_name, description_.trial))
      return false;
    file_name.append(clmc_file_name);

    const unsigned int TRAJECTORY_LENGTH = messages_.size();
    double trajectory_duration = (messages_[TRAJECTORY_LENGTH-1].header.stamp - messages_[0].header.stamp).toSec();
    if (TRAJECTORY_LENGTH == 1) // TODO: think about this again...
    {
      ROS_WARN("Only >%i< data sample contained when writing out CLMC data file.", (int)TRAJECTORY_LENGTH);
      trajectory_duration = 1.0;
    }
    ROS_ASSERT_MSG(trajectory_duration > 0.0, "Trajectory duration >%f< of trajectory named >%s< must be positive.",
                   trajectory_duration, file_name.c_str());
    const double SAMPLING_FREQUENCY = (double)TRAJECTORY_LENGTH / trajectory_duration;
    boost::scoped_ptr<dmp_lib::Trajectory> trajectory(new dmp_lib::Trajectory());
    std::vector<std::string> variable_names;
    variable_names.push_back("ros_time");
    variable_names.insert(variable_names.end(), messages_[0].names.begin(), messages_[0].names.end());
    if (!trajectory->initialize(variable_names, SAMPLING_FREQUENCY, true, (int)TRAJECTORY_LENGTH))
      return false;
    std::vector<double> data(variable_names.size(), 0.0);
    for (unsigned int i = 0; i < TRAJECTORY_LENGTH; ++i)
    {
      // TODO: fix this... make trajectory a float container
      data[0] = static_cast<float>(messages_[i].header.stamp.toSec());
      for (unsigned int j = 0; j < messages_[i].data.size(); ++j)
        data[1 + j] = messages_[i].data[j];
      if (!trajectory->add(data, true))
        return false;
    }
    return trajectory->writeToCLMCFile(file_name, true);
  }

//template<class MessageType>
//  bool TaskRecorderIO<MessageType>::writeStatistics(std::vector<std::vector<task_recorder2_msgs::AccumulatedTrialStatistics> >& vector_of_accumulated_trial_statistics)
//  {
//    ROS_ASSERT_MSG(initialized_, "Task recorder IO module is not initialized.");
//    std::string file_name = task_recorder2_utilities::getPathNameIncludingTrailingSlash(absolute_data_directory_path_)
//      + task_recorder2_utilities::getStatFileName(prefixed_topic_name_, description_.trial);
//    try
//    {
//      rosbag::Bag bag;
//      bag.open(file_name, rosbag::bagmode::Write);
//      for (unsigned int i = 0; i < vector_of_accumulated_trial_statistics.size(); ++i)
//      {
//        std::vector<task_recorder2_msgs::AccumulatedTrialStatistics> accumulated_trial_statistics = vector_of_accumulated_trial_statistics[i];
//        for (unsigned int j = 0; j < accumulated_trial_statistics.size(); ++j)
//        {
//          // accumulated_trial_statistics[j].id = getId(description_);
//          bag.write(topic_name_, messages_[j].header.stamp, accumulated_trial_statistics[j]);
//        }
//      }
//      bag.close();
//    }
//    catch (rosbag::BagIOException& e)
//    {
//      ROS_ERROR("Problem when writing to bag file named >%s< : %s", file_name.c_str(), e.what());
//      return false;
//    }
//    return true;
//  }

template<class MessageType>
  bool TaskRecorderIO<MessageType>::getList(std::vector<std::string>& descriptions)
  {
    boost::filesystem::path path = boost::filesystem::path(data_directory_name_);
    return task_recorder2_utilities::getDirectoryList(path, descriptions);
  }

}

#endif /* TASK_RECORDER_IO_H_ */
