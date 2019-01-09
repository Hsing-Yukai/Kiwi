//
// Created by kiwi on 19-1-7.
//

#include "Epoll.h"

Kiwi::Epoll::Epoll(Kiwi::EventLoop *event_loop) :
		_epoll_fd_(epoll_create(INIT_EVENT_LIST_SIZE)),
		_owner_event_loop_(event_loop),
		_event_list_(INIT_EVENT_LIST_SIZE) {}

void Kiwi::Epoll::poll(Kiwi::Epoll::ChannelList &active_channels)
{
	flag:
	int event_count = epoll_wait(_epoll_fd_, _event_list_.begin().base(), static_cast<int>(_event_list_.size()),
								 -1);
	if (event_count < 0)
	{
		switch (errno)
		{
			case EINTR:
				goto flag;
			default:
				std::cerr << "Epoll poll error : " << errno << " " << strerror(errno) << std::endl;
				std::terminate();
		}
	} else
	{
		for (int i = 0; i < event_count; i++)
		{
			auto *channel = reinterpret_cast<Channel *>(_event_list_[i].data.ptr);
			channel->set_revents(_event_list_[i].events);
			active_channels.emplace_back(channel);
		}
	}
}

void Kiwi::Epoll::add_channel(const Kiwi::Channel &channel)
{
	int fd = channel.get_fd();
	assert(_channels_.find(fd) == _channels_.end());
	_channels_.emplace(fd, channel);
	epoll_add(_channels_[fd]);
}

void Kiwi::Epoll::remove_channel(const Kiwi::Channel &channel)
{
	int fd = channel.get_fd();
	assert(_channels_.find(fd) != _channels_.end());
	_channels_.erase(fd);
	epoll_del(_channels_[fd]);
}

void Kiwi::Epoll::update_channel(const Kiwi::Channel &channel)
{
	int fd = channel.get_fd();
	assert(_channels_.find(fd) != _channels_.end());
	_channels_[fd]->set_events(channel.get_events());
	epoll_mod(_channels_[fd]);
}

Kiwi::Epoll::~Epoll()
{
	close(_epoll_fd_);
}

void Kiwi::Epoll::epoll_add(std::shared_ptr<Channel> channel)
{
	int fd = channel->get_fd();
	struct epoll_event event{};
	event.data.ptr = reinterpret_cast<void *>(channel.get());
	event.events = channel->get_events();
	int retval = epoll_ctl(_epoll_fd_, EPOLL_CTL_ADD, fd, &event);
	if (retval < 0)
	{
		switch (errno)
		{
			case EEXIST:
				return;
			default:
				std::cerr << "Epoll epoll_add error : " << errno << " " << strerror(errno) << std::endl;
				std::terminate();
		}
	}
}

void Kiwi::Epoll::epoll_del(std::shared_ptr<Channel> channel)
{
	int fd = channel->get_fd();
	struct epoll_event event{};
	event.data.ptr = reinterpret_cast<void *>(channel.get());
	event.events = channel->get_events();
	int retval = epoll_ctl(_epoll_fd_, EPOLL_CTL_DEL, fd, &event);
	if (retval < 0)
	{
		std::cerr << "Epoll epoll_del error : " << errno << " " << strerror(errno) << std::endl;
		std::terminate();
	}
}

void Kiwi::Epoll::epoll_mod(std::shared_ptr<Channel> channel)
{
	int fd = channel->get_fd();
	struct epoll_event event{};
	event.data.ptr = reinterpret_cast<void *>(channel.get());
	event.events = channel->get_events();
	int retval = epoll_ctl(_epoll_fd_, EPOLL_CTL_MOD, fd, &event);
	if (retval < 0)
	{
		std::cerr << "Epoll epoll_mod error : " << errno << " " << strerror(errno) << std::endl;
		std::terminate();
	}
}