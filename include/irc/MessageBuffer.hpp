/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MessageBuffer.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 21:52:11 by akacprzy          #+#    #+#             */
/*   Updated: 2026/01/18 21:57:30 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MESSAGEBUFFER_HPP

# define MESSAGEBUFFER_HPP

#include <string>
#include <vector>

// MessageBuffer class - manages incomplete messages
// Accumulates data until complete messages (ending with CRLF) are found
class MessageBuffer {
    private:
        // Accumulated data
        std::string buffer_;
        
        // Helper: find next complete message in buffer
        size_t findMessageEnd(size_t startPos = 0) const;

        public:
        // Constructor
        MessageBuffer();
        
        // Destructor
        ~MessageBuffer();
        
        // Append raw data to buffer
        void append(const std::string& data);
        
        // Extract complete messages (ending with \r\n)
        // Returns vector of complete messages, leaves incomplete data in buffer
        std::vector<std::string> extractMessages();
        
        // Get current buffer contents (for debugging)
        const std::string& getBuffer() const;
        
        // Clear buffer
        void clear();
        
        // Check if buffer is empty
        bool isEmpty() const;
        
        // Get buffer size
        size_t size() const;
};

#endif
