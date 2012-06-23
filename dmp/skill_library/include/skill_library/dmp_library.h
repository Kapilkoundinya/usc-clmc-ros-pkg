/*********************************************************************
  Computational Learning and Motor Control Lab
  University of Southern California
  Prof. Stefan Schaal 
 *********************************************************************
  \remarks		...
 
  \file		dmp_library.h

  \author	Peter Pastor
  \date		Jan 22, 2011

 *********************************************************************/

#ifndef DMP_LIBRARY_H_
#define DMP_LIBRARY_H_

// system includes
#include <string>
#include <map>
#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>
#include <algorithm>

#include <ros/ros.h>
#include <ros/package.h>

#include <usc_utilities/assert.h>
#include <usc_utilities/param_server.h>
#include <usc_utilities/file_io.h>

#include <dynamic_movement_primitive/dynamic_movement_primitive.h>
#include <dynamic_movement_primitive/dynamic_movement_primitive_io.h>

// local includes
#include <skill_library/dmp_library_io.h>

namespace skill_library
{

static const std::string SLASH = "/";
static const std::string BAG_FILE_ENDING = ".bag";

template<class DMPType, class MessageType>
class DMPLibrary
{

  public:

    /*! Constructor
     */
    DMPLibrary() :
      initialized_(false) {};

    /*! Destructor
     */
    virtual ~DMPLibrary() {};

    /*!
     * @param data_directory_name
     * @return
     */
    bool initialize(const std::string& data_directory_name);

    /*! Retreives the DMP from the library
     * @param name
     * @param dmp_message
     * @return
     */
    bool getDMP(const std::string& name,
                MessageType& dmp_message);

    /*! Adds DMP to library and sets the DMP id
     * @param name
     * @param dmp_message
     * @return
     */
    bool addDMP(MessageType& dmp_message,
                const std::string& name);

    /*! Adds DMP to library and sets the DMP id
     * @param name
     * @param dmp_message
     * @return
     */
    bool addDMP(typename DMPType::DMPPtr& dmp,
                const std::string& name);

    /*! Reloads all DMPs from disc into a buffer
     * @return True on success, otherwise False
     */
    bool reload();

    /*!
     * @return
     */
    bool print();

    /*!
     * @param name
     * @return converted absolute bag file name
     */
    std::string getBagFileName(const std::string& name)
    {
      return absolute_library_directory_path_.file_string() + SLASH + name + BAG_FILE_ENDING;
    }

  private:

    /*!
     * @param filename
     * @return
     */
    std::string getName(const std::string& filename)
    {
      std::string name = "INVALID_FILENAME";
      std::string f = filename;
      size_t slash_separater_pos;
      slash_separater_pos = f.find_last_of(SLASH);
      if (slash_separater_pos != std::string::npos)
      {
        size_t sp = slash_separater_pos + SLASH.length();
        size_t length = f.length() - sp;
        std::string filename = f.substr(sp, length);
        sp = filename.find_last_of(BAG_FILE_ENDING);
        name = filename.substr(0, sp - BAG_FILE_ENDING.length() + 1);
      }
      return name;
    }

    /*!
     */
    bool initialized_;

    /*!
     */
    std::map<std::string, MessageType> map_;

    /*! Sets the DMP id according to the provided name
     * @param msg
     * @param name
     * @return True on success, otherwise False
     */
    bool add(MessageType& msg, const std::string& name);

    /*!
     * @param msg
     * @param name
     * @return True on success, otherwise False
     */
    bool get(MessageType& msg, const std::string& name);

    /*!
     */
    boost::filesystem::path absolute_library_directory_path_;

};

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::initialize(const std::string& data_directory_name)
  {
    std::string library_directory_name = data_directory_name + DMPType::getVersionString();
    absolute_library_directory_path_ = boost::filesystem::path(library_directory_name);
    ROS_INFO("Initializing DMP library with path >%s<.", absolute_library_directory_path_.file_string().c_str());
    try
    {
      boost::filesystem::create_directories(absolute_library_directory_path_);
    }
    catch (std::exception& ex)
    {
      ROS_ERROR("Library directory >%s< could not be created: %s.", absolute_library_directory_path_.file_string().c_str(), ex.what());
      return false;
    }
    return (initialized_ = reload());
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::reload()
  {
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    std::vector<std::string> filenames;
    for (boost::filesystem::directory_iterator itr(absolute_library_directory_path_); itr != end_itr; ++itr)
    {
      filenames.push_back(itr->path().file_string());
    }
    std::sort(filenames.begin(), filenames.end());
    for (int i = 0; i < (int)filenames.size(); ++i)
    {
      MessageType dmp_message;
      if (!usc_utilities::FileIO<MessageType>::readFromBagFile(dmp_message, DMPType::getVersionString(), filenames[i], false))
      {
        ROS_ERROR("Problems reading >%s<. Cannot reload DMP library from disc.", filenames[i].c_str());
        return false;
      }
      std::string name = getName(filenames[i]);
      if (!add(dmp_message, name))
      {
        ROS_ERROR("Problems adding >%s<. Cannot reload DMP library from disc.", name.c_str());
        return false;
      }
    }
    return true;
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::print()
  {
    typename std::map<std::string, MessageType>::iterator it;
    ROS_INFO_COND(map_.empty(), "Libray buffer is empty.");
    ROS_INFO_COND(!map_.empty(), "Libray buffer contains:");
    int index = 1;
    for(it = map_.begin(); it != map_.end(); ++it)
    {
      ROS_INFO("(%i) >%s< has id >%i<.", index, it->first.c_str(), it->second.dmp.parameters.id);
      index++;
    }
    return true;
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::add(MessageType& msg, const std::string& name)
  {
    typename std::map<std::string, MessageType>::iterator it;
    int index = 1; // start at one
    bool found = false;
    for (it = map_.begin(); !found && it != map_.end(); ++it)
    {
      if (it->first.compare(name) == 0)
      {
        ROS_INFO("Overwriting DMP >%s<, but not changing id >%i<.", name.c_str(), it->second.dmp.parameters.id);
        msg.dmp.parameters.id = it->second.dmp.parameters.id;
        it->second = msg;
        found = true;
      }
      index++;
    }
    if (!found)
    {
      if (msg.dmp.parameters.id > 0)
      {
        ROS_INFO("Adding DMP >%s< and not changing id >%i<.", name.c_str(), msg.dmp.parameters.id);
      }
      else
      {
        ROS_INFO("Adding DMP >%s< and changing id from >%i< to >%i<.", name.c_str(), msg.dmp.parameters.id, index);
        msg.dmp.parameters.id = index;
      }
      map_.insert(typename std::pair<std::string, MessageType>(name, msg));
    }
    return true;
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::get(MessageType& msg, const std::string& name)
  {
    typename std::map<std::string, MessageType>::iterator it = map_.find(name);
    if (it == map_.end())
    {
      return false;
    }
    ROS_INFO("Found DMP >%s< with id >%i<.", name.c_str(), it->second.dmp.parameters.id);
    msg = it->second;
    return true;
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::addDMP(MessageType& dmp_message,
                                                const std::string& name)
  {
    if(name.empty())
    {
      ROS_ERROR("Cannot add DMP without name. Name must be specified.");
      return false;
    }
    if(!add(dmp_message, name))
    {
      return false;
    }
    std::string filename = getBagFileName(name);
    ROS_DEBUG("Writing into DMP Library at >%s<.", filename.c_str());
    return dmp::DynamicMovementPrimitiveIO<DMPType, MessageType>::writeToDisc(dmp_message, filename, false);
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::addDMP(typename DMPType::DMPPtr& dmp,
                                                const std::string& name)
  {
    if (name.empty())
    {
      ROS_ERROR("Cannot add DMP without name. Name must be specified.");
      return false;
    }
    std::string filename = getBagFileName(name);
    MessageType dmp_message;
    if(!dmp->writeToMessage(dmp_message))
    {
      return false;
    }
    if(!add(dmp_message, name))
    {
      return false;
    }
    ROS_DEBUG("Writing into DMP Library at >%s<.", filename.c_str());
    return dmp::DynamicMovementPrimitiveIO<DMPType, MessageType>::writeToDisc(dmp_message, filename);
  }

template<class DMPType, class MessageType>
  bool DMPLibrary<DMPType, MessageType>::getDMP(const std::string& name,
                                                MessageType& dmp_message)
  {
    // check whether it is in the cache.
    if(get(dmp_message, name))
    {
      return true;
    }
    std::string filename = getBagFileName(name);
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for (boost::filesystem::directory_iterator itr(absolute_library_directory_path_); itr != end_itr; ++itr)
    {
      ROS_DEBUG("Checking: >%s< and >%s<.", itr->path().file_string().c_str(), filename.c_str());
      if(filename.compare(itr->path().file_string()) == 0)
      {
        if(!usc_utilities::FileIO<MessageType>::readFromBagFile(dmp_message, DMPType::getVersionString(), filename, false))
        {
          ROS_ERROR("Problems reading >%s<. Cannot return DMP.", filename.c_str());
          return false;
        }
        if(!add(dmp_message, name))
        {
          ROS_WARN("Could not add DMP >%s< to local cache. Don't care though...", name.c_str());
        }
        return true;
      }
    }
    ROS_ERROR("Could not find DMP with name >%s<.", name.c_str());
    return false;
  }
}

#endif /* DMP_LIBRARY_H_ */
