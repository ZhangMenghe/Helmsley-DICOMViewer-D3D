const static float colorbar_tex_width = 256.0, colorbar_tex_height=120.0;

const static float3 ORGAN_COLORS[7] = {
	float3(0.24, 0.004, 0.64), float3(0.008, 0.278, 0.99), float3(0.75, 0.634, 0.996),
	float3(1, 0.87, 0.14), float3(0.98, 0.88, 1.0), float3(0.99, 0.106, 0.365), float3(.0, 0.314, 0.75)
};

const static int COLOR_SCHEME_HEX[4][256] = {
		{0xcf0000,0xcf0000,0xcf0000,0xd00000,0xd00000,0xd00000,0xd00000,0xcf0000,0xcf0000,0xce0000,0xce0000,0xcd0000,0xcc0000,0xcb0100,0xc91200,0xc81c00,0xc62400,0xc52b00,0xc33100,0xc13700,0xbf3c00,0xbd4100,0xbb4500,0xb94a00,0xb74e00,0xb45100,0xb25500,0xb05800,0xad5c00,0xab5f00,0xa86200,0xa66500,0xa36800,0xa16b00,0x9e6d00,0x9b7000,0x997300,0x967500,0x937700,0x907a00,0x8e7c00,0x8b7e00,0x888000,0x858200,0x828400,0x7e8600,0x7b8800,0x778a00,0x748c00,0x708e00,0x6c9000,0x689200,0x649400,0x609500,0x5b9700,0x569900,0x519a00,0x4c9c00,0x479e00,0x419f00,0x3ba100,0x33a200,0x2ba300,0x22a500,0x15a600,0x00a800,0x00a900,0x00aa00,0x00ab00,0x00ac00,0x00ae00,0x00af00,0x00b000,0x00b100,0x00b200,0x00b300,0x00b400,0x00b500,0x00b500,0x00b600,0x00b700,0x00b800,0x00b900,0x00b900,0x00ba00,0x00bb00,0x00bb00,0x00bc00,0x00bd00,0x00bd00,0x00be00,0x00bf00,0x00bf00,0x00c000,0x00c000,0x00c100,0x00c200,0x00c200,0x00c300,0x00c300,0x00c400,0x00c414,0x00c522,0x00c52d,0x00c637,0x00c63f,0x00c647,0x00c74e,0x00c756,0x00c85c,0x00c863,0x00c869,0x00c970,0x00c976,0x00c97c,0x00c982,0x00ca89,0x00ca8f,0x00ca95,0x00ca9a,0x00caa0,0x00cba6,0x00cbac,0x00cbb2,0x00cbb8,0x00cbbd,0x00cbc3,0x00cbc9,0x00cbce,0x00cad3,0x00cad9,0x00c9de,0x00c8e3,0x00c8e8,0x00c7ee,0x00c7f3,0x00c6f8,0x00c5fe,0x14c4ff,0x21c3ff,0x2ac3ff,0x32c2ff,0x38c1ff,0x3dc0ff,0x42bfff,0x47beff,0x4abdff,0x4ebcff,0x51bbff,0x54baff,0x56b9ff,0x58b8ff,0x5ab7ff,0x5cb6ff,0x5db5ff,0x5eb4ff,0x5fb3ff,0x5fb3ff,0x5fb2ff,0x5fb2ff,0x5fb1ff,0x5eb1ff,0x5eb1ff,0x5db1ff,0x5cb1ff,0x5bb1ff,0x5bb1ff,0x5ab1ff,0x5ab2ff,0x5ab2ff,0x5ab3ff,0x5eb3ff,0x61b3ff,0x66b4ff,0x6bb4ff,0x71b3ff,0x77b3ff,0x7eb3ff,0x85b2ff,0x8db1ff,0x94b0ff,0x9cafff,0xa4aeff,0xabadff,0xb3acff,0xbaaaff,0xc2a9ff,0xc9a7ff,0xd0a6ff,0xd7a4ff,0xdda2ff,0xe4a1ff,0xea9fff,0xf09dff,0xf69cff,0xfc9aff,0xff98ff,0xff96ff,0xff95ff,0xff93ff,0xff91ff,0xff8fff,0xff8eff,0xff8cff,0xff8aff,0xff89ff,0xff87ff,0xff85ff,0xff83ff,0xff82ff,0xff80ff,0xff7eff,0xff7cff,0xff7cff,0xff7dff,0xff7fff,0xff80ff,0xff81ff,0xff82ff,0xff84ff,0xff85ff,0xff86ff,0xff88ff,0xff89ff,0xff8aff,0xff8bff,0xff8dff,0xff8eff,0xff8fff,0xff90fb,0xff92f6,0xff93f0,0xff94eb,0xff96e6,0xff97e0,0xff98db,0xff99d5,0xff9bd0,0xff9ccb,0xff9dc5,0xff9ec0,0xff9fbb,0xffa1b6,0xffa2b2,0xffa3ad,0xffa4a9,0xffa5a5,0xffa6a1,0xffa79e,0xffa89b,0xffa998,0xffaa96,0xffab95,0xffab94,0xffac93,0xffad92},
		{0x4020ff,0x3d24ff,0x3926ff,0x3529ff,0x302bff,0x2c2eff,0x2830ff,0x2332ff,0x1f34ff,0x1a35ff,0x1537ff,0x1138ff,0x0c3aff,0x063bff,0x003cff,0x003eff,0x013fff,0x0240ff,0x0342ff,0x0443ff,0x0645ff,0x0747ff,0x0948ff,0x0b4aff,0x0c4cff,0x0e4eff,0x0f50ff,0x1052ff,0x1154ff,0x1156ff,0x1258ff,0x125aff,0x115cff,0x115fff,0x1061ff,0x0f63ff,0x0d65ff,0x0b67ff,0x0869ff,0x046cff,0x006eff,0x0070fd,0x0072fb,0x0074f8,0x0076f6,0x0078f4,0x007af1,0x007cef,0x007eec,0x0080ea,0x0082e7,0x0084e5,0x0086e2,0x0087e0,0x0089dd,0x008bdb,0x008dd8,0x008fd6,0x0090d3,0x0092d1,0x0094ce,0x0095cc,0x0097c9,0x0098c7,0x009ac4,0x009bc2,0x009dbf,0x009fbd,0x00a0ba,0x00a1b8,0x00a3b5,0x00a4b3,0x00a6b0,0x00a7ae,0x00a9ab,0x00aaa9,0x00aba6,0x00aca3,0x00ada0,0x00ae9d,0x00af9a,0x00b097,0x00b194,0x00b291,0x00b38e,0x00b48b,0x00b588,0x00b685,0x00b782,0x00b87f,0x00b97c,0x00ba79,0x00bb75,0x00bc72,0x00bc6f,0x00bd6b,0x00be68,0x00bf64,0x00c061,0x00c15d,0x00c259,0x00c356,0x00c352,0x00c44e,0x00c54a,0x00c646,0x00c741,0x00c73d,0x00c838,0x00c933,0x00ca2e,0x00cb28,0x00cb21,0x00cc19,0x00cd0f,0x00ce00,0x00ce00,0x00cf00,0x00d000,0x00d100,0x00d100,0x00d200,0x00d300,0x00d400,0x00d400,0x00d500,0x00d600,0x00d600,0x00d700,0x00d800,0x00d900,0x00d900,0x00da00,0x00db00,0x00db00,0x00dc00,0x00dd00,0x00dd00,0x00de00,0x00de00,0x00df00,0x00e000,0x00e000,0x00e100,0x00e100,0x00e200,0x00e200,0x00e300,0x00e300,0x00e400,0x00e400,0x10e400,0x1de500,0x26e500,0x2ee500,0x35e600,0x3be600,0x41e600,0x47e700,0x4ce700,0x52e700,0x57e700,0x5ce700,0x61e800,0x65e800,0x6ae800,0x6fe800,0x73e800,0x78e800,0x7ce800,0x80e800,0x85e800,0x89e800,0x8de800,0x92e800,0x96e800,0x9ae800,0x9ee800,0xa2e700,0xa6e700,0xaae700,0xaee700,0xb2e700,0xb6e600,0xbae600,0xbee600,0xc2e500,0xc6e500,0xc9e500,0xcde400,0xd1e400,0xd5e400,0xd8e300,0xdce300,0xe0e200,0xe3e200,0xe7e100,0xeae100,0xeee000,0xf1df00,0xf5df00,0xf8de00,0xfcdd00,0xffdd01,0xffdc13,0xffdb1d,0xffda25,0xffd92b,0xffd831,0xffd736,0xffd63b,0xffd53f,0xffd443,0xffd347,0xffd24a,0xffd14d,0xffd051,0xffcf54,0xffce57,0xffcc59,0xffcb5c,0xffca5f,0xffc861,0xffc764,0xffc666,0xffc468,0xffc36b,0xffc26d,0xffc06f,0xffbf71,0xffbd73,0xffbc75,0xffbb76,0xffb978,0xffb87a,0xffb77c,0xffb57d,0xffb47f,0xffb380,0xffb282,0xffb183,0xffb084,0xffaf86,0xffaf87,0xffae88,0xffad89,0xffad8a,0xffac8b,0xffac8c,0xffac8d,0xffac8e,0xffac8f,0xffac90,0xffac91,0xffad91,0xffad92},
		{0x000000, 0x060000, 0x0d0000, 0x120000, 0x160000, 0x190000, 0x1c0000, 0x1f0000, 0x220000, 0x240000, 0x260000, 0x280000, 0x2b0000, 0x2d0000, 0x2e0000, 0x300000, 0x320000, 0x340000, 0x350000, 0x370000, 0x380000, 0x3a0000, 0x3b0000, 0x3d0000, 0x3e0000, 0x400000, 0x410000, 0x430000, 0x440000, 0x460000, 0x470000, 0x490000, 0x4a0000, 0x4c0000, 0x4d0000, 0x4f0000, 0x500000, 0x520000, 0x530000, 0x550000, 0x560000, 0x580000, 0x590100, 0x5b0100, 0x5d0100, 0x5e0100, 0x600100, 0x610100, 0x630100, 0x650100, 0x660100, 0x680100, 0x690100, 0x6b0100, 0x6d0100, 0x6e0100, 0x700100, 0x710100, 0x730100, 0x750100, 0x760100, 0x780200, 0x7a0200, 0x7b0200, 0x7d0200, 0x7f0200, 0x800200, 0x820200, 0x840200, 0x850200, 0x870200, 0x890200, 0x8a0200, 0x8c0300, 0x8e0300, 0x900300, 0x910300, 0x930300, 0x950300, 0x960300, 0x980300, 0x9a0300, 0x9c0300, 0x9d0400, 0x9f0400, 0xa10400, 0xa20400, 0xa40400, 0xa60400, 0xa80400, 0xa90400, 0xab0500, 0xad0500, 0xaf0500, 0xb00500, 0xb20500, 0xb40500, 0xb60600, 0xb80600, 0xb90600, 0xbb0600, 0xbd0600, 0xbf0700, 0xc00700, 0xc20700, 0xc40700, 0xc60800, 0xc80800, 0xc90800, 0xcb0800, 0xcd0900, 0xcf0900, 0xd10900, 0xd20a00, 0xd40a00, 0xd60a00, 0xd80b00, 0xda0b00, 0xdb0c00, 0xdd0c00, 0xdf0d00, 0xe10d00, 0xe30e00, 0xe40f00, 0xe60f00, 0xe81000, 0xea1100, 0xeb1300, 0xed1400, 0xee1600, 0xf01800, 0xf11b00, 0xf21d00, 0xf32000, 0xf52300, 0xf62600, 0xf62900, 0xf72c00, 0xf82f00, 0xf93200, 0xf93500, 0xfa3800, 0xfa3b00, 0xfb3d00, 0xfb4000, 0xfb4300, 0xfc4600, 0xfc4900, 0xfc4b00, 0xfd4e00, 0xfd5100, 0xfd5300, 0xfd5600, 0xfd5800, 0xfe5b00, 0xfe5d00, 0xfe5f00, 0xfe6200, 0xfe6400, 0xfe6600, 0xfe6800, 0xfe6b00, 0xfe6d00, 0xfe6f00, 0xfe7100, 0xfe7300, 0xfe7500, 0xfe7700, 0xfe7900, 0xfe7c00, 0xff7e00, 0xff8000, 0xff8200, 0xff8300, 0xff8500, 0xff8700, 0xff8900, 0xff8b00, 0xff8d00, 0xff8f00, 0xff9100, 0xff9300, 0xff9400, 0xff9600, 0xff9800, 0xff9a00, 0xff9c00, 0xff9d00, 0xff9f00, 0xffa100, 0xffa300, 0xffa401, 0xffa601, 0xffa801, 0xffaa01, 0xffab01, 0xffad01, 0xffaf01, 0xffb001, 0xffb202, 0xffb402, 0xffb502, 0xffb702, 0xffb902, 0xffba02, 0xffbc03, 0xffbd03, 0xffbf03, 0xffc103, 0xffc204, 0xffc404, 0xffc604, 0xffc704, 0xffc905, 0xffca05, 0xffcc05, 0xffce06, 0xffcf06, 0xffd106, 0xffd207, 0xffd407, 0xffd508, 0xffd708, 0xffd909, 0xffda09, 0xffdc0a, 0xffdd0a, 0xffdf0b, 0xffe00b, 0xffe20c, 0xffe30d, 0xffe50e, 0xffe60f, 0xffe810, 0xffea11, 0xffeb12, 0xffed14, 0xffee17, 0xfff01a, 0xfff11e, 0xfff324, 0xfff42a, 0xfff532, 0xfff73b, 0xfff847, 0xfff953, 0xfffb62, 0xfffb72, 0xfffc83, 0xfffd95, 0xfffea8, 0xfffeba, 0xfffecc, 0xfffede, 0xfffeee, 0xffffff},
		{0x000e5c,0x000f5e,0x000f60,0x011061,0x011063,0x011165,0x021166,0x031268,0x031269,0x04126b,0x05136c,0x07136e,0x081470,0x091471,0x0b1473,0x0d1574,0x0e1576,0x101677,0x121678,0x13167a,0x15177b,0x17177d,0x19177e,0x1b187f,0x1d1880,0x1f1882,0x211883,0x231984,0x251985,0x271986,0x291988,0x2b1a89,0x2d1a8a,0x2f1a8b,0x311a8c,0x341a8c,0x361a8d,0x391a8e,0x3b1a8f,0x3d1a8f,0x401a90,0x431a90,0x451a91,0x481a91,0x4b1991,0x4e1992,0x501992,0x531892,0x561892,0x591792,0x5b1792,0x5e1692,0x601692,0x631692,0x651592,0x681592,0x6a1492,0x6d1491,0x6f1391,0x711391,0x741291,0x761291,0x781191,0x7a1191,0x7c1090,0x7f1090,0x810f90,0x830f90,0x850e90,0x870e8f,0x890d8f,0x8b0d8f,0x8d0c8e,0x8f0c8e,0x910c8e,0x930b8d,0x950b8d,0x970b8d,0x990a8c,0x9b0a8c,0x9d0a8c,0x9f098b,0xa1098b,0xa3098a,0xa5098a,0xa7098a,0xa90989,0xaa0989,0xac0988,0xae0988,0xb00988,0xb20987,0xb30a87,0xb50a86,0xb70a86,0xb90b85,0xba0b85,0xbc0c85,0xbe0c84,0xbf0d84,0xc10d83,0xc30e83,0xc40e82,0xc60f82,0xc81081,0xc91181,0xcb1180,0xcd1280,0xce137f,0xd0147f,0xd1167e,0xd3177e,0xd4187d,0xd61a7c,0xd71b7c,0xd81d7b,0xda1e7a,0xdb2079,0xdc2179,0xde2378,0xdf2577,0xe02676,0xe12876,0xe22a75,0xe32c74,0xe42d73,0xe62f72,0xe73171,0xe83370,0xe9356f,0xe9366e,0xea386d,0xeb3a6c,0xec3c6b,0xed3e6a,0xee4069,0xee4267,0xef4466,0xf04565,0xf14764,0xf14963,0xf24b62,0xf24d61,0xf34f60,0xf3515f,0xf4525e,0xf4545d,0xf5565c,0xf5585b,0xf65a5a,0xf65c59,0xf65e58,0xf75f57,0xf76157,0xf76356,0xf86555,0xf86754,0xf86953,0xf86a52,0xf86c52,0xf86e51,0xf87050,0xf87250,0xf8744f,0xf8754e,0xf8774e,0xf8794d,0xf87b4c,0xf87d4c,0xf87e4b,0xf8804a,0xf8824a,0xf88349,0xf88548,0xf88748,0xf88847,0xf88a47,0xf88c46,0xf88d45,0xf98f45,0xf99044,0xf99243,0xf99343,0xf99542,0xf99641,0xf99841,0xf99940,0xf99b3f,0xf99c3f,0xfa9e3e,0xfa9f3d,0xfaa03d,0xfaa23c,0xfaa33b,0xfaa53b,0xfba63a,0xfba73a,0xfba939,0xfbaa39,0xfbab38,0xfbad38,0xfcae38,0xfcb037,0xfcb137,0xfcb237,0xfcb437,0xfcb537,0xfcb637,0xfcb837,0xfcb937,0xfcbb37,0xfcbc38,0xfcbd38,0xfcbf38,0xfcc038,0xfcc238,0xfcc339,0xfcc439,0xfcc639,0xfcc73a,0xfcc83a,0xfcca3a,0xfccb3b,0xfccd3b,0xfbce3b,0xfbcf3c,0xfbd13c,0xfbd23d,0xfbd33d,0xfbd53e,0xfbd63e,0xfbd83f,0xfad93f,0xfada40,0xfadc40,0xfadd41,0xfade41,0xf9e042,0xf9e142,0xf9e243,0xf9e444,0xf9e544,0xf8e745,0xf8e845,0xf8e946,0xf8eb47,0xf7ec47,0xf7ed48,0xf7ef48,0xf7f049,0xf6f14a,0xf6f34a,0xf6f44b,0xf5f64c,0xf5f74d,0xf5f84d},
};

cbuffer volumeSetupConstBuffer : register(b0) {
	uint4 u_tex_size;

	//opacity widget
	float4 u_opacity[30];
	int u_widget_num;
	int u_visible_bits;

	//contrast
	float u_contrast_low;
	float u_contrast_high;
	float u_brightness;
	float u_base_value;

	//mask
	uint u_maskbits;
	uint u_organ_num;
	bool u_mask_recolor;

	//others
	bool u_show_organ;
	bool u_show_annotation;
	uint u_color_scheme;//COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT COLOR_FIRE COLOR_CET_L08
	float u_annotate_rate;
};
// All components are in the range [0�1], including hue.
float3 hsv2rgb(float3 c) {
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y) * c.z;
}

float3 transfer_scheme(float gray) {
	return hsv2rgb(float3(gray, 1.0, 1.0));
}

float3 transfer_scheme(int cat, float gray) {
	return ORGAN_COLORS[cat - 1] * gray;
}

//hot to color. H(0~180)
float3 bright_scheme(float gray) {
	return hsv2rgb(float3((1.0 - gray) * 180.0 / 255.0, 1.0, 1.0));
}

float3 hex2rgb(int hex) {
	return float3(
		(float((hex >> 16) & 0xFF)) / 255.0,
		(float((hex >> 8) & 0xFF)) / 255.0,
		(float((hex) & 0xFF)) / 255.0
		);
}

float3 AdjustContrastBrightness(float3 color) {
	float cr = 1.0 / (u_contrast_high - u_contrast_low);
	color = clamp(cr * (color - u_contrast_low), .0, 1.0);
	return clamp(color + u_brightness * 2.0 - 1.0, .0, 1.0);
}