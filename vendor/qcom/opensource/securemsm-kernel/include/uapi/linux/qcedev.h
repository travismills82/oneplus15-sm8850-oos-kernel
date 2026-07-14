/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _UAPI_QCEDEV__H
#define _UAPI_QCEDEV__H

#include <linux/types.h>
#include <linux/ioctl.h>

#define QCEDEV_MAX_SHA_BLOCK_SIZE	64
#define QCEDEV_MAX_BEARER	31
#define QCEDEV_MAX_KEY_SIZE	64
#define QCEDEV_MAX_IV_SIZE	32
#define QCEDEV_MAX_MAC_SIZE	64

#define QCEDEV_MAX_BUFFERS      16
#define QCEDEV_MAX_SHA_DIGEST	32

#define QCEDEV_USE_PMEM		1
#define QCEDEV_NO_PMEM		0

#define QCEDEV_AES_KEY_128	16
#define QCEDEV_AES_KEY_192	24
#define QCEDEV_AES_KEY_256	32

/**
 *qcedev_oper_enum: Operation types
 * @QCEDEV_OPER_ENC:		Encrypt
 * @QCEDEV_OPER_DEC:		Decrypt
 * @QCEDEV_OPER_ENC_NO_KEY:	Encrypt. Do not need key to be specified by
 *				user. Key already set by an external processor.
 * @QCEDEV_OPER_DEC_NO_KEY:	Decrypt. Do not need the key to be specified by
 *				user. Key already set by an external processor.
 */
enum qcedev_oper_enum {
	QCEDEV_OPER_DEC		= 0,
	QCEDEV_OPER_ENC		= 1,
	QCEDEV_OPER_DEC_NO_KEY	= 2,
	QCEDEV_OPER_ENC_NO_KEY	= 3,
	QCEDEV_OPER_LAST
};

/**
 *qcedev_offload_oper_enum: Offload operation types (uses pipe keys)
 * @QCEDEV_OFFLOAD_HLOS_HLOS:   Non-secure to non-secure (eg. audio dec).
 * @QCEDEV_OFFLOAD_HLOS_CPB:    Non-secure to secure (eg. video dec).
 * @QCEDEV_OFFLOAD_CPB_HLOS:    Secure to non-secure (eg. hdcp video enc).
 */
enum qcedev_offload_oper_enum {
	QCEDEV_OFFLOAD_HLOS_HLOS = 1,
	QCEDEV_OFFLOAD_HLOS_HLOS_1 = 2,
	QCEDEV_OFFLOAD_HLOS_CPB = 3,
	QCEDEV_OFFLOAD_HLOS_CPB_1 = 4,
	QCEDEV_OFFLOAD_CPB_HLOS = 5,
	QCEDEV_OFFLOAD_OPER_LAST
};

/**
 *qcedev_offload_err_enum: Offload error conditions
 * @QCEDEV_OFFLOAD_NO_ERROR:        Successful crypto operation.
 * @QCEDEV_OFFLOAD_GENERIC_ERROR:   Generic error in crypto status.
 * @QCEDEV_OFFLOAD_KEY_TIMER_EXPIRED_ERROR:     Pipe key timer expired.
 * @QCEDEV_OFFLOAD_KEY_PAUSE_ERROR:     Pipe key pause (means GPCE is paused).
 * @QCEDEV_OFFLOAD_KEY_INDEX_OUT_OF_BOUNDS_ERROR: Key index is too large.
 * @QCEDEV_OFFLOAD_KEY_EMPTY_ERROR:   No key was provided.
 * @QCEDEV_OFFLOAD_KEY_USAGE_ERROR:   Key policy mismatch with request.
 *                                    Ex: Key is CTR key, but CBC was attempted.
 * @QCEDEV_OFFLOAD_KEY_SIZE_ERROR:  Provided key length did not match key size.
 * @QCEDEV_OFFLOAD_KEY_PERM_ERROR:  This Execution Environment is not authorized
 *                                  to use this key.
 */
enum qcedev_offload_err_enum {
	QCEDEV_OFFLOAD_NO_ERROR = 0,
	QCEDEV_OFFLOAD_GENERIC_ERROR = 1,
	QCEDEV_OFFLOAD_KEY_TIMER_EXPIRED_ERROR = 2,
	QCEDEV_OFFLOAD_KEY_PAUSE_ERROR = 3,
	QCEDEV_OFFLOAD_KEY_INDEX_OUT_OF_BOUNDS_ERROR = 4,
	QCEDEV_OFFLOAD_KEY_EMPTY_ERROR = 5,
	QCEDEV_OFFLOAD_KEY_USAGE_ERROR = 6,
	QCEDEV_OFFLOAD_KEY_SIZE_ERROR = 7,
	QCEDEV_OFFLOAD_KEY_PERM_ERROR = 8,
};

/**
 *qcedev_oper_enum: Cipher algorithm types
 * @QCEDEV_ALG_DES:		DES
 * @QCEDEV_ALG_3DES:		3DES
 * @QCEDEV_ALG_AES:		AES
 */
enum qcedev_cipher_alg_enum {
	QCEDEV_ALG_DES		= 0,
	QCEDEV_ALG_3DES		= 1,
	QCEDEV_ALG_AES		= 2,
	QCEDEV_ALG_LAST
};

/**
 *qcedev_cipher_mode_enum : AES mode
 * @QCEDEV_AES_MODE_CBC:		CBC
 * @QCEDEV_AES_MODE_ECB:		ECB
 * @QCEDEV_AES_MODE_CTR:		CTR
 * @QCEDEV_AES_MODE_XTS:		XTS
 * @QCEDEV_AES_MODE_CCM:		CCM
 * @QCEDEV_DES_MODE_CBC:		CBC
 * @QCEDEV_DES_MODE_ECB:		ECB
 */
enum qcedev_cipher_mode_enum {
	QCEDEV_AES_MODE_CBC	= 0,
	QCEDEV_AES_MODE_ECB	= 1,
	QCEDEV_AES_MODE_CTR	= 2,
	QCEDEV_AES_MODE_XTS	= 3,
	QCEDEV_AES_MODE_CCM	= 4,
	QCEDEV_DES_MODE_CBC	= 5,
	QCEDEV_DES_MODE_ECB	= 6,
	QCEDEV_AES_DES_MODE_LAST
};

/**
 *enum qcedev_sha_alg_enum : Secure Hashing Algorithm
 * @QCEDEV_ALG_SHA1:		Digest returned: 20 bytes (160 bits)
 * @QCEDEV_ALG_SHA256:		Digest returned: 32 bytes (256 bit)
 * @QCEDEV_ALG_SHA1_HMAC:	HMAC returned 20 bytes (160 bits)
 * @QCEDEV_ALG_SHA256_HMAC:	HMAC returned 32 bytes (256 bit)
 * @QCEDEV_ALG_AES_CMAC:		Configurable MAC size
 */
enum qcedev_sha_alg_enum {
	QCEDEV_ALG_SHA1		= 0,
	QCEDEV_ALG_SHA256	= 1,
	QCEDEV_ALG_SHA1_HMAC	= 2,
	QCEDEV_ALG_SHA256_HMAC	= 3,
	QCEDEV_ALG_AES_CMAC	= 4,
	QCEDEV_ALG_SHA_ALG_LAST
};

/**
 * struct buf_info - Buffer information
 * @offset:			Offset from the base address of the buffer
 *				(Used when buffer is allocated using PMEM)
 * @vaddr:			Virtual buffer address pointer
 * @len:				Size of the buffer
 */
struct	buf_info {
	union {
		__u32	offset;
		__u8		*vaddr;
	};
	__u32	len;
};

/**
 * struct buf64_info - Buffer information - 64 bit
 * @len:				Size of the buffer
 * @offset:	Offset from the base address of the buffer
 *				(Used when buffer is allocated using PMEM)
 * @vaddr:			Virtual buffer address pointer
 */
struct	buf_info_64 {
	__u64	len;
	union {
		__u64	offset;
		__u8	*vaddr;
	};
};

#define QCEDEV_KEY_TYPE_NO_KEY           0 /* Cipher does not use keys. */
#define QCEDEV_KEY_TYPE_LEGACY_PIPE_KEY  1 /* Implied by offload_op in req. */
#define QCEDEV_KEY_TYPE_DRM_KEY_INDEX    2 /* Use key_index field. */
#define QCEDEV_KEY_TYPE_GP_KEY_INDEX     3 /* (WIP) Use key_index field. */
#define QCEDEV_KEY_TYPE_SOFTWARE_KEY     4 /* (WIP) Use software_key field.*/

/**
 * struct qcedev_key - generic key for qcedev
 * @key_length (IN):     Length of the key in bytes. Always needed. NO_KEY = 0.
 * @key_type (IN):       One of QCEDEV_KEY_TYPE_*.
 *                       Determines which key is used below
 * @software_key (IN):   Software key for the operation.
 * @key_index (OUT):     Index of hardware key in the GPCE Key Table.
 */
struct qcedev_key {
	__u32 key_length;
	__u32 key_type;
	union {
		__u8 software_key[QCEDEV_MAX_KEY_SIZE]; /* Not yet supported. Field is reserved. */
		__u32 key_index;
	};
};

/**
 * struct qcedev_vbuf_info - Source and destination Buffer information
 * @src:				Array of buf_info for input/source
 * @dst:				Array of buf_info for output/destination
 */
struct	qcedev_vbuf_info {
	struct buf_info	src[QCEDEV_MAX_BUFFERS];
	struct buf_info	dst[QCEDEV_MAX_BUFFERS];
};

/**
 * struct qcedev_vbuf_info_64 - Source and destination Buffer information 64 bit
 * @src:			Array of buf_info for input/source
 * @dst:			Array of buf_info for output/destination
 * @entries:			Size of the Array of src/dst.
 * @_padding_204:		Padding to maintain 8 byte alignment.
 */
struct	qcedev_vbuf_info_64 {
	struct buf_info_64	src[QCEDEV_MAX_BUFFERS];
	struct buf_info_64	dst[QCEDEV_MAX_BUFFERS];
	__u32 entries;
	__u32 _padding_204;
};

/**
 * struct qcedev_pmem_info - Stores PMEM buffer information
 * @fd_src:			Handle to /dev/adsp_pmem used to allocate
 *				memory for input/src buffer
 * @src:				Array of buf_info for input/source
 * @fd_dst:			Handle to /dev/adsp_pmem used to allocate
 *				memory for output/dst buffer
 * @dst:				Array of buf_info for output/destination
 * @pmem_src_offset:		The offset from input/src buffer
 *				(allocated by PMEM)
 */
struct	qcedev_pmem_info {
	int		fd_src;
	struct buf_info	src[QCEDEV_MAX_BUFFERS];
	int		fd_dst;
	struct buf_info	dst[QCEDEV_MAX_BUFFERS];
};

/**
 * struct qcedev_cipher_op_req - Holds the ciphering request information
 * @use_pmem (IN):	Flag to indicate if buffer source is PMEM
 *			QCEDEV_USE_PMEM/QCEDEV_NO_PMEM
 * @pmem (IN):		Stores PMEM buffer information.
 *			Refer struct qcedev_pmem_info
 * @vbuf (IN/OUT):	Stores Source and destination Buffer information
 *			Refer to struct qcedev_vbuf_info
 * @data_len (IN):	Total Length of input/src and output/dst in bytes
 * @in_place_op (IN):	Indicates whether the operation is inplace where
 *			source == destination
 *			When using PMEM allocated memory, must set this to 1
 * @enckey (IN):		128 bits of confidentiality key
 *			enckey[0] bit 127-120, enckey[1] bit 119-112,..
 *			enckey[15] bit 7-0
 * @encklen (IN):	Length of the encryption key(set to 128  bits/16
 *			bytes in the driver)
 * @iv (IN/OUT):		Initialisation vector data
 *			This is updated by the driver, incremented by
 *			number of blocks encrypted/decrypted.
 * @ivlen (IN):		Length of the IV
 * @byteoffset (IN):	Offset in the Cipher BLOCK (applicable and to be set
 *			for AES-128 CTR mode only)
 * @alg (IN):		Type of ciphering algorithm: AES/DES/3DES
 * @mode (IN):		Mode use when using AES algorithm: ECB/CBC/CTR
 *			Apllicabel when using AES algorithm only
 * @op (IN):		Type of operation: QCEDEV_OPER_DEC/QCEDEV_OPER_ENC or
 *			QCEDEV_OPER_ENC_NO_KEY/QCEDEV_OPER_DEC_NO_KEY
 *
 *If use_pmem is set to 0, the driver assumes that memory was not allocated
 * via PMEM, and kernel will need to allocate memory and copy data from user
 * space buffer (data_src/dta_dst) and process accordingly and copy data back
 * to the user space buffer
 *
 * If use_pmem is set to 1, the driver assumes that memory was allocated via
 * PMEM.
 * The kernel driver will use the fd_src to determine the kernel virtual address
 * base that maps to the user space virtual address base for the  buffer
 * allocated in user space.
 * The final input/src and output/dst buffer pointer will be determined
 * by adding the offsets to the kernel virtual addr.
 *
 * If use of hardware key is supported in the target, user can configure the
 * key parameters (encklen, enckey) to use the hardware key.
 * In order to use the hardware key, set encklen to 0 and set the enckey
 * data array to 0.
 */
struct	qcedev_cipher_op_req {
	__u8				use_pmem;
	union {
		struct qcedev_pmem_info	pmem;
		struct qcedev_vbuf_info	vbuf;
	};
	__u32			entries;
	__u32			data_len;
	__u8				in_place_op;
	__u8				enckey[QCEDEV_MAX_KEY_SIZE];
	__u32			encklen;
	__u8				iv[QCEDEV_MAX_IV_SIZE];
	__u32			ivlen;
	__u32			byteoffset;
	enum qcedev_cipher_alg_enum	alg;
	enum qcedev_cipher_mode_enum	mode;
	enum qcedev_oper_enum		op;
};

/**
 * struct qcedev_sha_op_req - Holds the hashing request information
 * @data (IN):			Array of pointers to the data to be hashed
 * @entries (IN):		Number of buf_info entries in the data array
 * @data_len (IN):		Length of data to be hashed
 * @digest (IN/OUT):		Returns the hashed data information
 * @diglen (OUT):		Size of the hashed/digest data
 * @authkey (IN):		Pointer to authentication key for HMAC
 * @authklen (IN):		Size of the authentication key
 * @alg (IN):			Secure Hash algorithm
 */
struct	qcedev_sha_op_req {
	struct buf_info			data[QCEDEV_MAX_BUFFERS];
	__u32			entries;
	__u32			data_len;
	__u8				digest[QCEDEV_MAX_SHA_DIGEST];
	__u32			diglen;
	__u8				*authkey;
	__u32			authklen;
	enum qcedev_sha_alg_enum	alg;
};

/**
 * struct pattern_info - Holds pattern information for pattern-based
 * decryption/encryption for AES ECB, counter, and CBC modes.
 * @patt_sz (IN):       Total number of blocks.
 * @proc_data_sz (IN):  Number of blocks to be processed.
 * @patt_offset (IN):   Start of the segment.
 */
struct pattern_info {
	__u8 patt_sz;
	__u8 proc_data_sz;
	__u8 patt_offset;
};

/**
 * struct qcedev_offload_cipher_op_req - Holds the offload request information
 *                                       Encryption key is stored in pipe, which
 *                                       is chosen through @op parameter.
 *                                       Equivalent to this key-type.
 *                                       QCEDEV_KEY_TYPE_LEGACY_PIPE_KEY
 * @vbuf (IN/OUT):      Stores Source and destination Buffer information.
 *                      Refer to struct qcedev_vbuf_info.
 * @entries (IN):       Number of entries to be processed as part of request.
 * @data_len (IN):      Total Length of input/src and output/dst in bytes
 * @in_place_op (IN):   Indicates whether the operation is inplace where
 *                      source == destination.
 * @encklen (IN):       Length of the encryption key(set to 128  bits/16
 *                      bytes in the driver).
 * @iv (IN/OUT):        Initialisation vector data
 *                      This is updated by the driver, incremented by
 *                      number of blocks encrypted/decrypted.
 * @ivlen (IN):         Length of the IV.
 * @iv_ctr_size (IN):   IV counter increment mask size.
 *                      Driver sets the mask value based on this size.
 * @byteoffset (IN):    Offset in the Cipher BLOCK (applicable and to be set
 *                      for AES-128 CTR mode only).
 * @block_offset (IN):  Offset in the block that needs a skip of encrypt/
 *                      decrypt.
 * @pattern_valid (IN): Indicates the request contains a valid pattern.
 * @pattern_info (IN):  The pattern to be used for the offload request.
 * @is_copy_op (IN):    Offload operations sometimes requires a copy between
 *                      secure and non-secure buffers without any encrypt/
 *                      decrypt operations.
 * @alg (IN):           Type of ciphering algorithm: AES/DES/3DES.
 * @mode (IN):          Mode use when using AES algorithm: ECB/CBC/CTR.
 *                      Applicable when using AES algorithm only.
 * @op (IN):            Type of operation.
 *                      Refer to qcedev_offload_oper_enum.
 * @err (OUT):          Error in crypto status.
 *                      Refer to qcedev_offload_err_enum.
 */
struct qcedev_offload_cipher_op_req {
	struct qcedev_vbuf_info vbuf;
	__u32 entries;
	__u32 data_len;
	__u32 in_place_op;
	__u32 encklen;
	__u8 iv[QCEDEV_MAX_IV_SIZE];
	__u32 ivlen;
	__u32 iv_ctr_size;
	__u32 byteoffset;
	__u8 block_offset;
	__u8 is_pattern_valid;
	__u8 is_copy_op;
	__u8 encrypt;
	struct pattern_info pattern_info;
	enum qcedev_cipher_alg_enum alg;
	enum qcedev_cipher_mode_enum mode;
	enum qcedev_offload_oper_enum op;
	enum qcedev_offload_err_enum err;
};

/**
 * struct qcedev_extended_cipher_req - Holds the request information
 * @vbuf (IN/OUT):      Stores Source and destination Buffer information.
 *                      Refer to struct qcedev_vbuf_info.
 * @byte_offset (IN):   Offset in the Cipher BLOCK (applicable and to be set
 *                      for AES-128 CTR mode only).
 * @encrypt (IN):       Indicates whether using encrypt or decrypt.
 * @pattern_valid (IN): Indicates the request contains a valid pattern.
 * @is_copy_op (IN):    Indicates an operation where data is not decrypted, only
 *                      copied. Ignored from secure to non-secure buffer to
 *                      protect sensitive data.
 * @in_place_op (IN):   Indicates whether the operation happens in-place,
 *                      src == dst.
 * @_padding_20d:	7 bytes of padding to align iv_ctr_size
 * @iv_ctr_size (IN):   IV counter increment mask size.
 *                      Driver sets the mask value based on this size.
 * @iv_len (IN):        Length of the IV in bytes.
 * @mac_len (IN)[WIP]:  Length of the MAC in bytes.
 * @iv (IN/OUT):        Initialization Vector (IV) data
 *                      This is updated by the driver, incremented by
 *                      number of blocks encrypted/decrypted.
 *                      AES-GCM only supports 12 byte (96 bit) IV.
 * @mac (IN/OUT)[WIP]:  Message Authentication Code (MAC). Input for decrypt,
 *                      output for encrypt. MAC is verified on decrypt.
 *                      ***CHECK THE ERR FIELD FOR MAC_ERROR***
 * @block_offset (IN):  Number of 16-byte blocks to skip. These blocks are not
 *                      encrypted and are simply copied from input to output.
 *                      Similar to AAD for non-AEAD ciphers. No authentication.
 *                      This is ignored for AEAD Ciphers (CCM/GCM).
 * @aad_len (IN)[WIP]:  Length of Additional Authenticated Data (AAD) at the
 *                      beginning of the data, unencrypted.
 *                      CCM requires that aad_len be a multiple of 16.
 *                      GCM does not have this restriction.
 *                      aad_len + implied_cipher_len = data_len.
 *                      Driver adds padding where needed.
 *                      Only used for AEAD Ciphers (CCM, GCM).
 *                      Ignored for other ciphers.
 * @data_len (IN):      Total Length of input/src and output/dst in bytes.
 * @key (IN):           Information about the key used for the operation.
 * @pattern_info (IN):  The pattern to be used for the request.
 * @_padding_2e3:       5 bytes of padding to align iv_ctr_size to align alg
 * @alg (IN):           Type of ciphering algorithm: AES/DES/3DES.
 * @mode (IN):          Mode use when using AES algorithm: ECB/CBC/CTR.
 *                      Applicable when using AES algorithm only.
 * @op (IN):            Type of operation.
 *                      Refer to qcedev_offload_oper_enum.
 * @err (OUT):          Error in crypto status.
 *                      Refer to qcedev_offload_err_enum.
 */

struct qcedev_extended_cipher_req {
	struct qcedev_vbuf_info_64 vbuf;
	__u8 byte_offset;
	__u8 encrypt;
	__u8 is_pattern_valid;
	__u8 is_copy_op;
	__u8 in_place_op;
	__u8 _padding_20d[7];
	__u32 iv_ctr_size;
	__u32 iv_len;
	__u32 mac_len;
	__u8 iv[QCEDEV_MAX_IV_SIZE];
	__u8 mac[QCEDEV_MAX_MAC_SIZE];
	__u64 block_offset;
	__u64 aad_len;
	__u64 data_len;
	struct qcedev_key key;
	struct pattern_info pattern_info;
	__u8 _padding_2e3[5];
	enum qcedev_cipher_alg_enum alg;
	enum qcedev_cipher_mode_enum mode;
	enum qcedev_offload_oper_enum op;
	enum qcedev_offload_err_enum err;
};

/**
 * struct qfips_verify_t - Holds data for FIPS Integrity test
 * @kernel_size  (IN):		Size of kernel Image
 * @kernel       (IN):		pointer to buffer containing the kernel Image
 */
struct qfips_verify_t {
	unsigned int kernel_size;
	void *kernel;
};

/**
 * struct qcedev_map_buf_req - Holds the mapping request information
 * fd (IN):            Array of fds.
 * num_fds (IN):       Number of fds in fd[].
 * fd_size (IN):       Array of sizes corresponding to each fd in fd[].
 * fd_offset (IN):     Array of offset corresponding to each fd in fd[].
 * vaddr (OUT):        Array of mapped virtual address corresponding to
 *			each fd in fd[].
 */
struct qcedev_map_buf_req {
	__s32         fd[QCEDEV_MAX_BUFFERS];
	__u32        num_fds;
	__u32        fd_size[QCEDEV_MAX_BUFFERS];
	__u32        fd_offset[QCEDEV_MAX_BUFFERS];
	__u64        buf_vaddr[QCEDEV_MAX_BUFFERS];
};

/**
 * struct qcedev_unmap_buf_req - Holds the hashing request information
 * fd (IN):            Array of fds to unmap
 * num_fds (IN):       Number of fds in fd[].
 */
struct  qcedev_unmap_buf_req {
	__s32         fd[QCEDEV_MAX_BUFFERS];
	__u32        num_fds;
};

struct file;

long qcedev_ioctl(struct file *file,
			unsigned int cmd, unsigned long arg);

#define QCEDEV_IOC_MAGIC	0x87

#define QCEDEV_IOCTL_ENC_REQ		\
	_IOWR(QCEDEV_IOC_MAGIC, 1, struct qcedev_cipher_op_req)
#define QCEDEV_IOCTL_DEC_REQ		\
	_IOWR(QCEDEV_IOC_MAGIC, 2, struct qcedev_cipher_op_req)
#define QCEDEV_IOCTL_SHA_INIT_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 3, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_SHA_UPDATE_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 4, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_SHA_FINAL_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 5, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_GET_SHA_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 6, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_LOCK_CE	\
	_IO(QCEDEV_IOC_MAGIC, 7)
#define QCEDEV_IOCTL_UNLOCK_CE	\
	_IO(QCEDEV_IOC_MAGIC, 8)
#define QCEDEV_IOCTL_GET_CMAC_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 9, struct qcedev_sha_op_req)
#define QCEDEV_IOCTL_MAP_BUF_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 10, struct qcedev_map_buf_req)
#define QCEDEV_IOCTL_UNMAP_BUF_REQ	\
	_IOWR(QCEDEV_IOC_MAGIC, 11, struct qcedev_unmap_buf_req)
#define QCEDEV_IOCTL_OFFLOAD_OP_REQ		\
	_IOWR(QCEDEV_IOC_MAGIC, 12, struct qcedev_offload_cipher_op_req)
#define QCEDEV_IOCTL_EXT_CIPHER_OP_REQ		\
	_IOWR(QCEDEV_IOC_MAGIC, 12, struct qcedev_extended_cipher_req)
#endif /* _UAPI_QCEDEV__H */
