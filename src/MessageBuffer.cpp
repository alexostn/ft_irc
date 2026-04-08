/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MessageBuffer.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 22:00:02 by akacprzy          #+#    #+#             */
/*   Updated: 2026/02/08 14:50:12 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "irc/MessageBuffer.hpp"

// Constructor - Initialize buffer_ as empty string
MessageBuffer::MessageBuffer() : buffer_("") {}

// Destructor - No cleanup needed (string handles it)
MessageBuffer::~MessageBuffer() {}

// Method - append() data to buffer_
void MessageBuffer::append(const std::string& data)
{
    buffer_ += data;
}

// Method - extractMessages()
// - Find all complete messages (ending with \r\n or \n)
// - Extract each complete message
// - Remove extracted messages from buffer_
// - Return vector of complete message strings
// - Keep incomplete data in buffer_ for next append
// RFC 2812 uses \r\n, but we tolerate bare \n for compatibility
std::vector<std::string> MessageBuffer::extractMessages()
{
    std::vector<std::string> messages;
    size_t pos;

    while ((pos = buffer_.find('\n')) != std::string::npos)
    {
        std::string msg = buffer_.substr(0, pos + 1);
        buffer_.erase(0, pos + 1);
        messages.push_back(msg);
    }
    return messages;
}

// Method - getBuffer() - return const reference to buffer_
const std::string& MessageBuffer::getBuffer() const
{
    return buffer_;
}

// Method - clear() - clear buffer_ string
void MessageBuffer::clear() 
{
    buffer_.clear();
}

// Method - isEmpty() - return buffer_.empty()
bool MessageBuffer::isEmpty() const
{
    return buffer_.empty();
}

// Method - size() - return buffer_.size()
size_t MessageBuffer::size() const 
{
    return buffer_.size();
}

// Method - findMessageEnd(size_t startPos) - helper method to find next line ending
size_t MessageBuffer::findMessageEnd(size_t startPos) const
{
    return buffer_.find('\n', startPos);
}
