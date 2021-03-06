/*
 * xfer-dcc.c - file transfer via DCC protocol
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-config.h"
#include "xfer-file.h"
#include "xfer-network.h"


/*
 * Child process for sending file with DCC protocol.
 */

void
xfer_dcc_send_file_child (struct t_xfer *xfer)
{
    int num_read, num_sent, blocksize;
    static char buffer[XFER_BLOCKSIZE_MAX];
    uint32_t ack;
    time_t last_sent, new_time, last_second, sent_ok;
    unsigned long long sent_last_second;

    blocksize = xfer->blocksize;
    if (weechat_config_integer (xfer_config_network_speed_limit) > 0)
    {
        if (blocksize > weechat_config_integer (xfer_config_network_speed_limit) * 1024)
            blocksize = weechat_config_integer (xfer_config_network_speed_limit) * 1024;
    }

    last_sent = time (NULL);
    last_second = time (NULL);
    sent_ok = 0;
    sent_last_second = 0;
    while (1)
    {
        /* read DCC ACK (sent by receiver) */
        if (xfer->pos > xfer->ack)
        {
            /* we should receive ACK for packets sent previously */
            while (1)
            {
                num_read = recv (xfer->sock, (char *) &ack, 4, MSG_PEEK);
                if ((num_read < 1) &&
                    ((num_read != -1) || ((errno != EAGAIN) && (errno != EWOULDBLOCK))))
                {
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_SEND_BLOCK);
                    return;
                }
                if (num_read == 4)
                {
                    recv (xfer->sock, (char *) &ack, 4, 0);
                    xfer->ack = ntohl (ack);

                    /* DCC send OK? */
                    if ((xfer->pos >= xfer->size)
                        && (xfer->ack >= xfer->size))
                    {
                        xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                                 XFER_NO_ERROR);
                        return;
                    }
                }
                else
                    break;
            }
        }

        /* send a block to receiver */
        if ((xfer->pos < xfer->size) &&
             (xfer->fast_send || (xfer->pos <= xfer->ack)))
        {
            if ((weechat_config_integer (xfer_config_network_speed_limit) > 0)
                && (sent_last_second >= (unsigned long long)weechat_config_integer (xfer_config_network_speed_limit) * 1024))
            {
                /* we're sending too fast (according to speed limit set by user) */
                usleep (100);
            }
            else
            {
                lseek (xfer->file, xfer->pos, SEEK_SET);
                num_read = read (xfer->file, buffer, blocksize);
                if (num_read < 1)
                {
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_READ_LOCAL);
                    return;
                }
                num_sent = send (xfer->sock, buffer, num_read, 0);
                if (num_sent < 0)
                {
                    /*
                     * socket is temporarily not available (receiver can't
                     * receive amount of data we sent ?!)
                     */
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        usleep (1000);
                    else
                    {
                        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                                 XFER_ERROR_SEND_BLOCK);
                        return;
                    }
                }
                if (num_sent > 0)
                {
                    xfer->pos += (unsigned long long) num_sent;
                    sent_last_second += (unsigned long long) num_sent;
                    new_time = time (NULL);
                    if ((last_sent != new_time)
                        || ((sent_ok == 0) && (xfer->pos >= xfer->size)))
                    {
                        last_sent = new_time;
                        xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                                                 XFER_NO_ERROR);
                        if (xfer->pos >= xfer->size)
                            sent_ok = new_time;
                    }
                }
            }
        }
        else
            usleep (1000);

        new_time = time (NULL);
        if (new_time > last_second)
        {
            last_second = new_time;
            sent_last_second = 0;
        }

        /*
         * if send if OK since 2 seconds or more, and that no ACK was received,
         * then consider it's OK
         */
        if ((sent_ok != 0) && (new_time > sent_ok + 2))
        {
            xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                     XFER_NO_ERROR);
            return;
        }
    }
}

/*
 * Sends ACK to sender using current position in file received.
 *
 * Returns:
 *   2: ACK sent successfully (the 4 bytes)
 *   1: ACK not sent, but we consider it's not a problem
 *   0: ACK not sent with socket error (DCC will be closed)
 */

int
xfer_dcc_recv_file_send_ack (struct t_xfer *xfer)
{
    int length, num_sent, total_sent;
    uint32_t pos;
    const void *ptr_buf;

    pos = htonl (xfer->pos);
    ptr_buf = &pos;
    length = 4;
    total_sent = 0;
    num_sent = send (xfer->sock, ptr_buf, length, 0);
    if (num_sent > 0)
        total_sent += num_sent;
    while (total_sent < length)
    {
        if ((num_sent == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
            return 0;

        /* if we can't send ACK now, just return with partial failure code */
        if (total_sent == 0)
            return 1;

        /* at least one byte has been sent, we must send whole ACK */
        usleep (1000);
        num_sent = send (xfer->sock, ptr_buf + total_sent,
                         length - total_sent, 0);
        if (num_sent > 0)
            total_sent += num_sent;
    }

    /* ACK successfully sent */
    return 2;
}

/*
 * Reads a resumed xfer from disk for hashing
 *
 * Returns 0 on success, 1 on failure
 */

static int
xfer_dcc_resume_hash(struct t_xfer *xfer)
{
    ssize_t end_pos = xfer->start_resume;
    const size_t buf_len = 1024*1024;
    char *buf = malloc(buf_len);
    ssize_t total_read = 0;
    int ret = 0;
    int fd = 0;

    if (!buf) return 1;

    while (fd <= 0)
    {
        fd = open(xfer->local_filename, O_RDONLY);
        if (fd < 0)
        {
            if (errno == EINTR) continue;
            fd = 0;
            ret = 1;
            break;
        }
    }

    if (fd)
    {
        while ( total_read < end_pos )
        {
            size_t to_read = end_pos - total_read;
            ssize_t num_read = 0;
            if (to_read > buf_len)
            {
                num_read = read(fd, buf, buf_len);
            } else {
                num_read = read(fd, buf, to_read);
            }
            if (num_read > 0)
            {
                gcry_md_write(xfer->hash_handle, buf, num_read);
                total_read += num_read;
            } else {
                if (num_read < 0)
                {
                    if (errno == EINTR) continue;
                    ret = 1;
                    break;
                }
            }
        }

        while( close(fd) < 0 )
        {
	        if (errno != EINTR) break;
        }
    }
    free(buf);

    return ret;
}

/*
 * Child process for receiving file with DCC protocol.
 */

void
xfer_dcc_recv_file_child (struct t_xfer *xfer)
{
    int flags, num_read, ack_enabled, ready;
    static char buffer[XFER_BLOCKSIZE_MAX];
    time_t last_sent, new_time;
    unsigned long long pos_last_ack;
    fd_set read_fds, write_fds, except_fds;

    /* If resuming, hash the portion of the file we have */
    if (xfer->start_resume > 0 && xfer->hash_handle)
    {
        xfer_network_write_pipe(xfer, XFER_STATUS_HASHING,
                                XFER_NO_ERROR);
        if (xfer_dcc_resume_hash(xfer))
        {
            gcry_md_close(xfer->hash_handle);
            xfer->hash_handle = NULL;
            xfer_network_write_pipe (xfer, XFER_STATUS_HASHING,
                                     XFER_ERROR_HASH_RESUME_ERROR);
        }
        xfer_network_write_pipe(xfer, XFER_STATUS_CONNECTING,
                                XFER_NO_ERROR);
    }

    /* first connect to sender (blocking) */
    if (!weechat_network_connect_to (xfer->proxy, xfer->sock,
                                     xfer->remote_address, xfer->port))
    {
        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                 XFER_ERROR_CONNECT_SENDER);
        return;
    }

    /* connection is OK, change DCC status (inform parent process) */
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                             XFER_NO_ERROR);

    /* make socket non-blocking */
    flags = fcntl (xfer->sock, F_GETFL);
    if (flags == -1)
        flags = 0;
    fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK);

    last_sent = time (NULL);
    ack_enabled = 1;
    pos_last_ack = 0;

    while (1)
    {
        /* wait until there is something to read on socket (or error) */
        FD_ZERO (&read_fds);
        FD_ZERO (&write_fds);
        FD_ZERO (&except_fds);
        FD_SET (xfer->sock, &read_fds);
        ready = select (xfer->sock + 1, &read_fds, &write_fds, &except_fds, NULL);
        if (ready == 0)
        {
            xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                     XFER_ERROR_RECV_BLOCK);
            return;
        }

        /* read maximum data on socket (until nothing is available) */
        while (1)
        {
            num_read = recv (xfer->sock, buffer, sizeof (buffer), 0);
            if (num_read == -1)
            {
                if ((errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR))
                {
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_RECV_BLOCK);
                    return;
                }
                /*
                 * no more data available on socket: exit loop, send ACK, and
                 * wait for new data on socket
                 */
                break;
            }
            else
            {
                ssize_t total_written = 0;

                if (num_read == 0)
                {
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_RECV_BLOCK);
                    return;
                }

                /* bytes received, write to disk */
                while (total_written < num_read)
                {
                    ssize_t written = write (xfer->file,
                                             buffer + total_written,
                                             num_read - total_written);
                    if ( written < 0 )
                    {
	                    if ( errno == EINTR ) continue;
                        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                                 XFER_ERROR_WRITE_LOCAL);
                        return;
                    } else {
                        if (xfer->hash_handle)
                        {
                            gcry_md_write (xfer->hash_handle,
                                           buffer + total_written,
                                           written);
                        }
                        total_written += written;
                    }
                }

                xfer->pos += (unsigned long long) num_read;

                /* file received OK? */
                if (xfer->pos >= xfer->size)
                {
                    /* Check hash and report result to pipe */
                    if (xfer->hash_handle)
                    {
                        unsigned char* bin_hash;
                        char hash[9];
                        gcry_md_final(xfer->hash_handle);
                        bin_hash = gcry_md_read(xfer->hash_handle, 0);
                        if (bin_hash)
                        {
                            snprintf(hash, 9, "%.2X%.2X%.2X%.2X", bin_hash[0],
                                     bin_hash[1], bin_hash[2], bin_hash[3]);
                            if (weechat_strcasecmp(hash, xfer->hash_target) == 0)
                            {
                                xfer_network_write_pipe (xfer, XFER_STATUS_HASHED,
                                                         XFER_NO_ERROR);
                            } else {
                                xfer_network_write_pipe (xfer, XFER_STATUS_HASHED,
                                                         XFER_ERROR_HASH_MISMATCH);
                            }
                        }
                    }

                    /*
                     * extra delay before sending ACK, otherwise the send of ACK
                     * may fail
                     */
                    fsync(xfer->file);
                    usleep (100000);

                    /* send ACK to sender without checking return code (file OK) */
                    xfer_dcc_recv_file_send_ack (xfer);

                    /* set status done and return */
                    xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                             XFER_NO_ERROR);
                    return;
                }

                /* update status of DCC (parent process) */
                new_time = time (NULL);
                if (last_sent != new_time)
                {
                    last_sent = new_time;
                    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                                             XFER_NO_ERROR);
                }
            }
        }

        /* send ACK to sender (if needed) */
        if (ack_enabled && (xfer->pos > pos_last_ack))
        {
            switch (xfer_dcc_recv_file_send_ack (xfer))
            {
                case 0:
                    /* send error, socket down? */
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_SEND_ACK);
                    return;
                case 1:
                    /* send error, not fatal (buffer full?): disable ACKs */
                    ack_enabled = 0;
                    break;
                case 2:
                    /* send OK: save position in file as last ACK sent */
                    pos_last_ack = xfer->pos;
                    break;
            }
        }
    }
}
