/*
 * security_sdk_ecc.h
 *
 *  Created on: Sep 17, 2018
 *      Author: ydoh
 */

#ifndef APPS_EXAMPLES_SECURITY_SDK_SECURITY_SDK_ECC_H_
#define APPS_EXAMPLES_SECURITY_SDK_SECURITY_SDK_ECC_H_

static unsigned char test_ecdsa_dev_bp[] = { 0x30, 0x78, 0x02, 0x01, 0x01, 0x04, 0x20, 0x1e, 0x20, 0xd3, 0xa6, 0xaa, 0x38, 0xf6, 0xf1, 0x65,
												0x19, 0xb8, 0xae, 0x31, 0x86, 0x7a, 0x47, 0x3b, 0xaf, 0x5e, 0x54, 0x93, 0xb1, 0x46, 0xba, 0x8d,
												0x39, 0x25, 0xa9, 0xe4, 0xd2, 0x41, 0x66, 0xa0, 0x0b, 0x06, 0x09, 0x2b, 0x24, 0x03, 0x03, 0x02,
												0x08, 0x01, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x1a, 0xbc, 0xb3, 0x24, 0xa4, 0x1e,
												0x89, 0x79, 0x6e, 0xbe, 0x75, 0x7f, 0x78, 0xa7, 0x32, 0x21, 0xae, 0x7b, 0xb5, 0xe4, 0xa4, 0x4d,
												0x07, 0x44, 0x0e, 0x07, 0x5a, 0x77, 0x3d, 0xb3, 0xd8, 0xfc, 0x65, 0x02, 0xfc, 0xd7, 0x1a, 0x3c,
												0xcb, 0x9a, 0x4e, 0x34, 0xde, 0x32, 0xe8, 0x3e, 0x2e, 0xda, 0x09, 0xe0, 0x5f, 0x24, 0x7b, 0x86,
												0x83, 0x08, 0xc3, 0xf6, 0x7f, 0xe3, 0x81, 0xbb, 0xda, 0x62
											  };

static unsigned char test_ecdsa_521_dev_nist[] = { 0x30, 0x81, 0xdc, 0x02, 0x01, 0x01, 0x04, 0x42, 0x00, 0x94, 0x4f, 0x20, 0xa2, 0x5b, 0x75, 0xcb,
												  0x2d, 0xc6, 0x17, 0xd1, 0xd9, 0x04, 0xb5, 0x19, 0x4b, 0xfd, 0x80, 0x8f, 0xd8, 0x6f, 0xc0, 0x23,
												  0x2f, 0x23, 0x90, 0xe8, 0x29, 0xa9, 0x9e, 0x28, 0x0b, 0x8d, 0xa8, 0xf2, 0xb5, 0x4a, 0x7d, 0xc8,
												  0x09, 0x79, 0xb7, 0xb4, 0xc4, 0x6e, 0xb4, 0x95, 0x35, 0xbe, 0xcd, 0x19, 0x96, 0x9b, 0x3c, 0xae,
												  0x50, 0x2c, 0x98, 0x77, 0x47, 0x0c, 0x7f, 0x87, 0x3b, 0xbd, 0xa0, 0x07, 0x06, 0x05, 0x2b, 0x81,
												  0x04, 0x00, 0x23, 0xa1, 0x81, 0x89, 0x03, 0x81, 0x86, 0x00, 0x04, 0x00, 0xa7, 0xe3, 0x75, 0x81,
												  0x42, 0x82, 0x15, 0x7c, 0x36, 0xff, 0x46, 0x43, 0x78, 0x9f, 0x94, 0x99, 0x5c, 0xba, 0x59, 0x90,
												  0xdc, 0x2a, 0xed, 0x90, 0xbb, 0x87, 0x07, 0x99, 0x69, 0x17, 0xa5, 0x1d, 0x38, 0x71, 0xc9, 0x91,
												  0x0e, 0x9c, 0xa6, 0x35, 0x30, 0x91, 0x1e, 0x98, 0x07, 0x5c, 0xfd, 0xe2, 0x8c, 0x44, 0x16, 0xc3,
												  0xc4, 0x2b, 0x03, 0xf0, 0x3b, 0xd7, 0xdc, 0x26, 0xcb, 0x82, 0xe5, 0xcd, 0x47, 0x01, 0xc3, 0x01,
												  0xa9, 0xd6, 0xe3, 0xe3, 0x65, 0x4a, 0x8d, 0x23, 0xff, 0xb4, 0xaa, 0xa6, 0x32, 0x30, 0x63, 0x8c,
												  0x31, 0x1a, 0x83, 0xa7, 0xd6, 0xd1, 0xf1, 0xd2, 0x50, 0xbf, 0xd5, 0x51, 0x3b, 0x74, 0x13, 0x71,
												  0x79, 0x1b, 0x55, 0x34, 0x2b, 0xae, 0x98, 0x40, 0x32, 0xcc, 0xf4, 0xae, 0xc5, 0xe3, 0xd3, 0x03,
												  0xcf, 0x8f, 0x36, 0xeb, 0x2c, 0xc8, 0x0a, 0xc7, 0xad, 0x88, 0x24, 0x9f, 0x00, 0xd4, 0x18
												};

static unsigned char test_ecdsa_256_dev_nist[] = { 0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0xDA, 0x51, 0x77, 0x3A, 0xCB, 0x81, 0xAF, 0x18,
													  0x2A, 0xE4, 0x72, 0xBE, 0xD6, 0x76, 0x80, 0xFC, 0xE3, 0x87, 0x60, 0x31, 0x2B, 0x00, 0x4E,
													  0x8B, 0x1C, 0xAB, 0x12, 0x57, 0xE8, 0x86, 0xF5, 0xA1, 0xA0, 0x0A, 0x06, 0x08, 0x2A, 0x86,
													  0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0xA1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x6E, 0xFA, 0x3D,
													  0x85, 0xC6, 0x14, 0x9C, 0x77, 0x44, 0x65, 0xDC, 0x5C, 0x8B, 0xD8, 0xAE, 0x95, 0xD4, 0x50,
													  0x69, 0xAF, 0xD3, 0x85, 0x14, 0x00, 0x01, 0xEB, 0x37, 0x76, 0xE0, 0xCE, 0x68, 0x8C, 0x9E,
													  0x5C, 0x50, 0x18, 0x57, 0xA4, 0xFC, 0x03, 0x5A, 0x08, 0x0B, 0x4F, 0x89, 0x77, 0x3D, 0xD9,
													  0x55, 0xAF, 0x17, 0xD3, 0x6D, 0xA3, 0x75, 0xD3, 0xFE, 0x61, 0x42, 0xD0, 0xCB, 0xFB, 0xAA,
													  0xAC
													};

static unsigned char test_ecdsa_384_dev_nist[] = { 0x30, 0x81, 0xa4, 0x02, 0x01, 0x01, 0x04, 0x30, 0x48, 0x0c, 0x60, 0x20, 0xb0, 0x11, 0x11,
													  0x11, 0x6d, 0x41, 0xbb, 0x43, 0xf7, 0x21, 0xe4, 0x33, 0x8f, 0x6b, 0x0e, 0x88, 0x7e, 0xff,
													  0x84, 0xa8, 0x37, 0x76, 0xb2, 0x89, 0x71, 0xe7, 0x42, 0x3f, 0xaa, 0x49, 0x9c, 0x30, 0x9f,
													  0x13, 0xc9, 0xc8, 0x4f, 0x06, 0xfe, 0xad, 0x69, 0x5c, 0x1c, 0x94, 0xA0, 0x07, 0x06, 0x05,
													  0x2B, 0x81, 0x04, 0x00, 0x22, 0xA1, 0x64, 0x03, 0x62, 0x00, 0x04, 0x45, 0x59, 0x6D, 0x6E,
													  0x5C, 0x7A, 0x60, 0x72, 0xE2, 0xA2, 0xDE, 0x2D, 0x4C, 0x45, 0xAA, 0x2C, 0x74, 0xA5, 0x3A,
													  0x2C, 0x42, 0x04, 0xBD, 0x61, 0x87, 0x90, 0xDB, 0xCB, 0x4A, 0xD4, 0x02, 0xC6, 0xBE, 0x76,
													  0x44, 0x6B, 0xA7, 0x33, 0x96, 0xA5, 0x26, 0x77, 0x87, 0x1B, 0x79, 0xA7, 0xB2, 0x6A, 0xBA,
													  0x07, 0xDA, 0xE5, 0x6C, 0x5D, 0x12, 0x5A, 0x44, 0xDB, 0xC2, 0xB6, 0x0C, 0x13, 0xD0, 0xE6,
													  0x51, 0xA9, 0x01, 0x11, 0xA4, 0x5A, 0x52, 0x41, 0x61, 0xF0, 0x4F, 0x23, 0x7C, 0xAE, 0xBC,
													  0x07, 0xD7, 0x96, 0x7B, 0xD1, 0x7A, 0x62, 0xD8, 0xE1, 0x4D, 0xFB, 0xA1, 0x4F, 0xCC, 0x5C,
													  0x7E, 0x8A
													};

#endif /* APPS_EXAMPLES_SECURITY_SDK_SECURITY_SDK_ECC_H_ */