/*****************************************************************************\
 *  constants.h -
 *****************************************************************************
 *  Copyright (C) 2012-2013 Los Alamos National Security, LLC.
 *  Written by Jimmy Cao <Jimmy.Cao@emc.com>, Ralph Castain <rhc@open-mpi.org>
 *  All rights reserved.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.schedmd.com/slurmdocs/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#ifndef CONSTANTS_H_
#define CONSTANTS_H_


#define DYNALLOC_ERR_BASE             0 /* internal use only */

enum {
    DYNALLOC_SUCCESS                            = (DYNALLOC_ERR_BASE),

    DYNALLOC_ERROR                              = (DYNALLOC_ERR_BASE -  1),
    DYNALLOC_ERR_OUT_OF_RESOURCE                = (DYNALLOC_ERR_BASE -  2), /* fatal error */
    DYNALLOC_ERR_TEMP_OUT_OF_RESOURCE           = (DYNALLOC_ERR_BASE -  3), /* try again later */
    DYNALLOC_ERR_RESOURCE_BUSY                  = (DYNALLOC_ERR_BASE -  4),
    DYNALLOC_ERR_BAD_PARAM                      = (DYNALLOC_ERR_BASE -  5),  /* equivalent to MPI_ERR_ARG error code */
    DYNALLOC_ERR_FATAL                          = (DYNALLOC_ERR_BASE -  6),
    DYNALLOC_ERR_NOT_IMPLEMENTED                = (DYNALLOC_ERR_BASE -  7),
    DYNALLOC_ERR_NOT_SUPPORTED                  = (DYNALLOC_ERR_BASE -  8),
    DYNALLOC_ERR_INTERUPTED                     = (DYNALLOC_ERR_BASE -  9),
    DYNALLOC_ERR_WOULD_BLOCK                    = (DYNALLOC_ERR_BASE - 10),
    DYNALLOC_ERR_IN_ERRNO                       = (DYNALLOC_ERR_BASE - 11),
    DYNALLOC_ERR_UNREACH                        = (DYNALLOC_ERR_BASE - 12),
    DYNALLOC_ERR_NOT_FOUND                      = (DYNALLOC_ERR_BASE - 13),
    DYNALLOC_EXISTS                             = (DYNALLOC_ERR_BASE - 14), /* indicates that the specified object already exists */
    DYNALLOC_ERR_TIMEOUT                        = (DYNALLOC_ERR_BASE - 15),
    DYNALLOC_ERR_NOT_AVAILABLE                  = (DYNALLOC_ERR_BASE - 16),
    DYNALLOC_ERR_PERM                           = (DYNALLOC_ERR_BASE - 17), /* no permission */
    DYNALLOC_ERR_VALUE_OUT_OF_BOUNDS            = (DYNALLOC_ERR_BASE - 18),
    DYNALLOC_ERR_FILE_READ_FAILURE              = (DYNALLOC_ERR_BASE - 19),
    DYNALLOC_ERR_FILE_WRITE_FAILURE             = (DYNALLOC_ERR_BASE - 20),
    DYNALLOC_ERR_FILE_OPEN_FAILURE              = (DYNALLOC_ERR_BASE - 21),
    DYNALLOC_ERR_PACK_MISMATCH                  = (DYNALLOC_ERR_BASE - 22),
    DYNALLOC_ERR_PACK_FAILURE                   = (DYNALLOC_ERR_BASE - 23),
    DYNALLOC_ERR_UNPACK_FAILURE                 = (DYNALLOC_ERR_BASE - 24),
    DYNALLOC_ERR_UNPACK_INADEQUATE_SPACE        = (DYNALLOC_ERR_BASE - 25),
    DYNALLOC_ERR_UNPACK_READ_PAST_END_OF_BUFFER = (DYNALLOC_ERR_BASE - 26),
    DYNALLOC_ERR_TYPE_MISMATCH                  = (DYNALLOC_ERR_BASE - 27),
    DYNALLOC_ERR_OPERATION_UNSUPPORTED          = (DYNALLOC_ERR_BASE - 28),
    DYNALLOC_ERR_UNKNOWN_DATA_TYPE              = (DYNALLOC_ERR_BASE - 29),
    DYNALLOC_ERR_BUFFER                         = (DYNALLOC_ERR_BASE - 30),
    DYNALLOC_ERR_DATA_TYPE_REDEF                = (DYNALLOC_ERR_BASE - 31),
    DYNALLOC_ERR_DATA_OVERWRITE_ATTEMPT         = (DYNALLOC_ERR_BASE - 32),
    DYNALLOC_ERR_MODULE_NOT_FOUND               = (DYNALLOC_ERR_BASE - 33),
    DYNALLOC_ERR_TOPO_SLOT_LIST_NOT_SUPPORTED   = (DYNALLOC_ERR_BASE - 34),
    DYNALLOC_ERR_TOPO_SOCKET_NOT_SUPPORTED      = (DYNALLOC_ERR_BASE - 35),
    DYNALLOC_ERR_TOPO_CORE_NOT_SUPPORTED        = (DYNALLOC_ERR_BASE - 36),
    DYNALLOC_ERR_NOT_ENOUGH_SOCKETS             = (DYNALLOC_ERR_BASE - 37),
    DYNALLOC_ERR_NOT_ENOUGH_CORES               = (DYNALLOC_ERR_BASE - 38),
    DYNALLOC_ERR_INVALID_PHYS_CPU               = (DYNALLOC_ERR_BASE - 39),
    DYNALLOC_ERR_MULTIPLE_AFFINITIES            = (DYNALLOC_ERR_BASE - 40),
    DYNALLOC_ERR_SLOT_LIST_RANGE                = (DYNALLOC_ERR_BASE - 41),
    DYNALLOC_ERR_NETWORK_NOT_PARSEABLE          = (DYNALLOC_ERR_BASE - 42),
    DYNALLOC_ERR_SILENT                         = (DYNALLOC_ERR_BASE - 43),
    DYNALLOC_ERR_NOT_INITIALIZED                = (DYNALLOC_ERR_BASE - 44),
    DYNALLOC_ERR_NOT_BOUND                      = (DYNALLOC_ERR_BASE - 45)
};

#define DYNALLOC_ERR_MAX                (DYNALLOC_ERR_BASE - 100)

#endif /* CONSTANTS_H_ */
