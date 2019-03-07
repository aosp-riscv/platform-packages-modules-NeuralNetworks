#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

layout = BoolScalar("layout", False) # NHWC

# TEST 1: CONV_NCHW_1
i1 = Input("op1", "TENSOR_FLOAT32", "{1, 3, 3, 1}")
f1 = Parameter("op2", "TENSOR_FLOAT32", "{1, 2, 2, 1}", [.25, .25, .25, .25])
b1 = Parameter("op3", "TENSOR_FLOAT32", "{1}", [0])
o1 = Output("op4", "TENSOR_FLOAT32", "{1, 2, 2, 1}")
Model().Operation("CONV_2D", i1, f1, b1, 0, 0, 0, 0, 1, 1, 0, layout).To(o1)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i1: ("TENSOR_QUANT8_ASYMM", 0.5, 0),
    f1: ("TENSOR_QUANT8_ASYMM", 0.125, 0),
    b1: ("TENSOR_INT32", 0.0625, 0),
    o1: ("TENSOR_QUANT8_ASYMM", 0.125, 0)
})
channelQuant8 = DataTypeConverter().Identify({
    i1: ("TENSOR_QUANT8_ASYMM", 0.5, 0),
    f1: ("TENSOR_QUANT8_SYMM_PER_CHANNEL", 0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.125])),
    b1: ("TENSOR_INT32", 0.0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.0625], hide=True)),
    o1: ("TENSOR_QUANT8_ASYMM", 0.125, 0)
})

# Instantiate an example
example = Example({
    i1: [1.0, 1.0, 1.0, 1.0, 0.5, 1.0, 1.0, 1.0, 1.0],
    o1: [.875, .875, .875, .875]
}).AddNchw(i1, o1, layout).AddInput(f1, b1).AddVariations("relaxed", quant8, channelQuant8, "float16")


# TEST 2: CONV_NCHW_2
i2 = Input("op1", "TENSOR_FLOAT32", "{1, 3, 4, 1}")
f2 = Parameter("op2", "TENSOR_FLOAT32", "{1, 3, 3, 1}", [1, 4, 7, 2, 5, 8, 3, 6, 9])
b2 = Parameter("op3", "TENSOR_FLOAT32", "{1}", [-200])
o2 = Output("op4", "TENSOR_FLOAT32", "{1, 3, 4, 1}")
Model().Operation("CONV_2D", i2, f2, b2, 1, 1, 1, 1, layout).To(o2)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i2: ("TENSOR_QUANT8_ASYMM", 0.5, 127),
    f2: ("TENSOR_QUANT8_ASYMM", 0.5, 127),
    b2: ("TENSOR_INT32", 0.25, 0),
    o2: ("TENSOR_QUANT8_ASYMM", 1.0, 50)
})
channelQuant8 = DataTypeConverter().Identify({
    i2: ("TENSOR_QUANT8_ASYMM", 0.5, 127),
    f2: ("TENSOR_QUANT8_SYMM_PER_CHANNEL", 0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.5])),
    b2: ("TENSOR_INT32", 0.0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.25], hide=True)),
    o2: ("TENSOR_QUANT8_ASYMM", 1.0, 50)
})

# Instantiate an example
example = Example({
    i2: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12],
    o2: [0, 0, 0, 0, 35, 112, 157, 0, 0, 34, 61, 0]
}).AddNchw(i2, o2, layout).AddInput(f2, b2).AddVariations("relaxed", quant8, channelQuant8, "float16")


# TEST 3: CONV_NCHW_CHANNEL
i3 = Input("op1", "TENSOR_FLOAT32", "{1, 1, 1, 3}")
f3 = Parameter("op2", "TENSOR_FLOAT32", "{3, 1, 1, 3}", [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5])
b3 = Parameter("op3", "TENSOR_FLOAT32", "{3}", [0., 0., 0.])
o3 = Output("op4", "TENSOR_FLOAT32", "{1, 1, 1, 3}")
Model("channel").Operation("CONV_2D", i3, f3, b3, 0, 0, 0, 0, 1, 1, 0, layout).To(o3)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i3: ("TENSOR_QUANT8_ASYMM", 0.5, 0),
    f3: ("TENSOR_QUANT8_ASYMM", 0.5, 0),
    b3: ("TENSOR_INT32", 0.25, 0),
    o3: ("TENSOR_QUANT8_ASYMM", 0.5, 0)
})
channelQuant8 = DataTypeConverter().Identify({
    i3: ("TENSOR_QUANT8_ASYMM", 0.5, 0),
    f3: ("TENSOR_QUANT8_SYMM_PER_CHANNEL", 0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.5, 0.4, 0.3])),
    b3: ("TENSOR_INT32", 0.0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.25, 0.2, 0.15], hide=True)),
    o3: ("TENSOR_QUANT8_ASYMM", 0.5, 0)
})

# Instantiate an example
example = Example({
    i3: [5.,  5.,   5.],
    o3: [15., 37.5, 60.]
}).AddNchw(i3, o3, layout).AddInput(f3, b3).AddVariations("relaxed", quant8, channelQuant8, "float16")


# TEST 4: CONV_NCHW_LARGE
i4 = Input("op1", "TENSOR_FLOAT32", "{1, 2, 3, 3}")
f4 = Parameter("op2", "TENSOR_FLOAT32", "{3, 1, 1, 3}", [1., 4., 7., 2., 5., 8., 3., 6., 9.])
b4 = Parameter("op3", "TENSOR_FLOAT32", "{3}", [0., 0., 0.])
o4 = Output("op4", "TENSOR_FLOAT32", "{1, 2, 3, 3}")
Model("large").Operation("CONV_2D", i4, f4, b4, 0, 0, 0, 0, 1, 1, 0, layout).To(o4)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i4: ("TENSOR_QUANT8_ASYMM", 0.5, 128),
    f4: ("TENSOR_QUANT8_ASYMM", 0.5, 128),
    b4: ("TENSOR_INT32", 0.25, 0),
    o4: ("TENSOR_QUANT8_ASYMM", 2.0, 0)
})
channelQuant8 = DataTypeConverter().Identify({
    i4: ("TENSOR_QUANT8_ASYMM", 0.5, 128),
    f4: ("TENSOR_QUANT8_SYMM_PER_CHANNEL", 0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.5, 1.0, 0.5])),
    b4: ("TENSOR_INT32", 0.0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.25, 0.5, 0.25], hide=True)),
    o4: ("TENSOR_QUANT8_ASYMM", 2.0, 0)
})
channelQuant8_mult_gt_1 = DataTypeConverter().Identify({
    i4: ("TENSOR_QUANT8_ASYMM", 1.0, 127),
    f4: ("TENSOR_QUANT8_SYMM_PER_CHANNEL", 0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.5, 1.0, 1.005])),
    b4: ("TENSOR_INT32", 0.0, 0, SymmPerChannelQuantParams(channelDim=0, scales=[0.5, 1.0, 1.005], hide=True)),
    o4: ("TENSOR_QUANT8_ASYMM", 1.0, 127)
})

# Instantiate an example
example = Example({
    i4: [1.,   2.,   3.,   4.,   5.,   6.,   7.,   8.,   9.,
         10.,  11.,  12.,  13.,  14.,  15.,  16.,  17.,  18.],
    o4: [30.,   36.,   42.,
         66.,   81.,   96.,
         102.,  126.,  150.,
         138.,  171.,  204.,
         174.,  216.,  258.,
         210.,  261.,  312.]
}).AddNchw(i4, o4, layout).AddInput(f4, b4).AddVariations("relaxed", quant8, channelQuant8, channelQuant8_mult_gt_1, "float16")


# TEST 5/6: CONV_1_H3_W2_[SAME|VALID]
i5 = Input("op1", "TENSOR_FLOAT32", "{1, 8, 8, 3}")
f5 = Parameter("op2", "TENSOR_FLOAT32", "{1, 3, 2, 3}", [-0.966213, -0.467474, -0.82203, -0.579455, 0.0278809, -0.79946, -0.684259, 0.563238, 0.37289, 0.738216, 0.386045, -0.917775, 0.184325, -0.270568, 0.82236, 0.0973683, -0.941308, -0.144706])
b5 = Parameter("op3", "TENSOR_FLOAT32", "{1}", [0.])
o5 = Output("op4", "TENSOR_FLOAT32", "{1, 8, 8, 1}")
o6 = Output("op4", "TENSOR_FLOAT32", "{1, 6, 7, 1}")
model_1_same = Model("1_H3_W2_SAME").Operation("CONV_2D", i5, f5, b5, 1, 1, 1, 0, layout).To(o5)
model_1_valid = Model("1_H3_W2_VALID").Operation("CONV_2D", i5, f5, b5, 2, 1, 1, 0, layout).To(o6)

example = Example({
    i5: [-0.869931, 0.644628, -0.918393, 0.153672, 0.868562, -0.358177, -0.134931, -0.247565, 0.22174, -0.259157, -0.284296, -0.538065, 0.765559, 0.41986, -0.556241, 0.658494, 0.214355, -0.850169, -0.252893, -0.478935, 0.530526, -0.0700663, -0.988729, -0.303061, 0.150845, 0.829915, 0.476349, 0.406537, -0.355343, 0.757145, -0.356362, 0.800482, -0.713861, 0.210483, -0.634303, 0.718236, -0.752038, 0.457547, -0.550769, -0.551178, 0.446766, -0.227462, 0.216348, -0.852806, -0.351486, 0.55906, -0.668493, -0.303493, -0.363763, -0.162837, 0.0701012, 0.756097, -0.142269, 0.329724, -0.656317, -0.998086, -0.652949, -0.40316, -0.893682, 0.432744, 0.612362, -0.869588, -0.71327, -0.398092, -0.0423559, 0.436576, -0.925272, 0.176549, 0.822904, 0.096833, -0.296802, -0.427195, 0.031654, -0.254479, 0.244905, 0.0948254, 0.643769, -0.90391, 0.352665, -0.901179, 0.266159, -0.968068, -0.615401, -0.388975, 0.939052, -0.116289, 0.107523, -0.0582711, 0.435172, 0.334675, 0.459711, 0.717436, 0.496627, -0.680175, -0.415066, 0.339848, 0.506004, -0.337808, -0.107218, -0.172496, 0.870638, 0.931872, -0.953884, 0.903042, 0.760078, 0.209727, -0.285384, -0.45514, 0.113194, 0.0756611, 0.0924435, -0.472863, 0.960609, -0.160385, -0.839445, 0.457097, 0.163348, 0.344867, -0.131619, 0.688715, -0.540827, 0.571259, -0.95587, 0.506164, -0.155839, 0.0789621, 0.756772, -0.662069, 0.242908, 0.460821, 0.177872, -0.289839, -0.640603, 0.702598, -0.506406, -0.568262, -0.0713716, 0.413792, 0.159673, -0.305208, 0.133816, -0.160254, 0.787323, -0.753244, 0.600721, 0.263186, -0.162387, 0.477962, -0.702951, -0.731036, -0.939481, -0.524519, 0.934072, -0.511637, -0.503499, 0.106236, -0.323684, 0.534444, -0.843745, 0.364171, 0.0370358, -0.168801, -0.404559, -0.814178, 0.91745, -0.334276, 0.66925, -0.801201, 0.156511, -0.427949, 0.379153, 0.818597, -0.649902, 0.427087, -0.586015, -0.559789, -0.833923, 0.0892409, -0.621251, 0.213826, 0.465509, 0.4704, 0.380261, 0.413067, 0.180822, 0.172866, 0.59614, 0.825575, 0.662916, -0.704381, -0.297631, 0.697778],
    o5: [1.85284, -0.0393656, -0.127353, 1.43115, -0.302294, -1.0402, 0.655023, -0.587614, 1.72003, 1.55816, 0.667546, 2.23663, 0.0661516, 0.290254, 0.770222, -0.346357, -1.58197, -0.850595, -0.484224, 0.949967, -0.577263, -0.871949, 2.34132, -0.104506, -0.135965, -0.985713, 0.815147, 1.03114, -1.41915, -0.515534, -0.373639, 1.42026, -1.50604, 0.673113, 3.06139, -0.388578, -1.76707, -0.315667, -1.03815, -0.343435, 0.432787, -1.41643, 1.12944, -0.175806, -0.846415, 1.40095, 0.70832, -1.46717, 2.19562, -2.61266, -0.705383, 1.26124, 1.46545, -2.35761, 2.04494, 1.23741, -0.527402, -0.39954, -0.0128623, 1.3644, 0.985755, -0.718118, -0.1008, 1.24327]
}, {
    i5: [-0.295335, -0.00387601, -0.552251, 0.166084, -0.28482, -0.152143, -0.719885, -0.869386, -0.745598, 0.823947, 0.473183, -0.331337, 0.187631, 0.0426571, -0.826897, -0.755085, -0.472453, -0.0233656, 0.0483436, 0.933418, -0.961974, 0.0125783, 0.219742, 0.342604, -0.15166, 0.0934905, 0.783221, 0.129664, 0.838844, -0.271388, 0.924519, 0.342843, 0.274418, 0.350817, 0.841638, -0.543993, -0.00283395, -0.128467, -0.682943, -0.319117, 0.84634, 0.283003, 0.32865, 0.0293755, -0.0335696, 0.591266, -0.0743476, -0.741271, 0.462056, -0.583625, -0.590183, 0.6234, 0.535269, -0.670818, -0.955642, -0.770173, 0.479986, 0.664377, 0.399445, -0.968874, -0.276263, -0.901951, 0.544104, -0.958981, 0.482658, -0.807284, 0.305369, -0.947818, 0.827498, -0.382887, -0.805741, -0.796678, -0.299804, -0.229828, 0.818783, -0.103055, -0.45568, -0.227827, 0.543743, -0.96073, 0.946747, -0.857182, -0.96426, -0.292411, -0.715614, 0.765278, -0.475043, -0.590142, -0.238507, 0.673002, -0.473357, -0.319626, 0.936014, 0.486607, 0.580844, 0.425352, -0.800994, 0.290763, -0.494953, -0.441162, 0.718677, -0.828427, 0.96965, 7.53637e-05, -0.699973, -0.526886, -0.352682, 0.799466, 0.332789, 0.723389, 0.407659, -0.934084, -0.284705, 0.961484, -0.700395, -0.985808, -0.595342, -0.691721, 0.49448, -0.0842649, 0.0390966, 0.298938, -0.128094, -0.97158, 0.86393, 0.270606, -0.468986, -0.256605, 0.47215, -0.273117, -0.590343, -0.826529, -0.725381, -0.194821, -0.259661, -0.0949207, -0.180302, 0.0446834, -0.222133, -0.40393, 0.295772, -0.92949, 0.580079, -0.169856, 0.330311, 0.0173551, -0.635823, 0.475942, 0.907175, 0.242777, -0.512208, 0.362463, 0.0496289, 0.65171, 0.990057, 0.690733, -0.469013, -0.101311, -0.68372, -0.157841, -0.677711, -0.708224, -0.659437, -0.407607, 0.677033, 0.89032, 0.228307, -0.749514, 0.772958, 0.054701, 0.551705, 0.917052, -0.895022, -0.702397, 0.484142, 0.108648, 0.833347, 0.478872, -0.984112, 0.387176, -0.73299, 0.7526, 0.443312, -0.0987856, 0.125415, 0.10876, -0.498108, 0.43209, 0.344609, 0.928941, -0.130732, -0.0569167],
    o5: [-0.000614278, -1.21221, 0.443861, 0.102117, -2.52714, 1.47489, 0.173474, -0.237577, 1.28735, 1.91315, 2.51734, 0.375841, 0.637563, 2.653, 2.72959, -1.6271, 1.17389, -2.12119, 2.91417, -2.24246, 0.0497045, -0.127107, -0.144473, -0.133762, -0.393284, -2.02346, -0.239178, -0.246508, 1.29277, 1.32963, 0.117521, 1.22372, 0.0665713, 1.09438, -1.31426, 2.52594, -0.969211, 0.515478, -1.60926, -0.838905, 0.135211, 0.786415, -1.14382, -0.739102, -1.01731, 0.281615, 2.36311, 0.891823, 1.93872, -0.150491, 3.45217, 2.28219, 1.18282, -2.25086, 3.05468, 0.166228, 0.434554, -2.57529, -0.958662, -2.23978, 2.66776, 0.542601, 1.76107, -1.08134]
}, model=model_1_same).AddNchw(i5, o5, layout).AddVariations("relaxed", "float16")

example = Example({
    i5: [-0.869931, 0.644628, -0.918393, 0.153672, 0.868562, -0.358177, -0.134931, -0.247565, 0.22174, -0.259157, -0.284296, -0.538065, 0.765559, 0.41986, -0.556241, 0.658494, 0.214355, -0.850169, -0.252893, -0.478935, 0.530526, -0.0700663, -0.988729, -0.303061, 0.150845, 0.829915, 0.476349, 0.406537, -0.355343, 0.757145, -0.356362, 0.800482, -0.713861, 0.210483, -0.634303, 0.718236, -0.752038, 0.457547, -0.550769, -0.551178, 0.446766, -0.227462, 0.216348, -0.852806, -0.351486, 0.55906, -0.668493, -0.303493, -0.363763, -0.162837, 0.0701012, 0.756097, -0.142269, 0.329724, -0.656317, -0.998086, -0.652949, -0.40316, -0.893682, 0.432744, 0.612362, -0.869588, -0.71327, -0.398092, -0.0423559, 0.436576, -0.925272, 0.176549, 0.822904, 0.096833, -0.296802, -0.427195, 0.031654, -0.254479, 0.244905, 0.0948254, 0.643769, -0.90391, 0.352665, -0.901179, 0.266159, -0.968068, -0.615401, -0.388975, 0.939052, -0.116289, 0.107523, -0.0582711, 0.435172, 0.334675, 0.459711, 0.717436, 0.496627, -0.680175, -0.415066, 0.339848, 0.506004, -0.337808, -0.107218, -0.172496, 0.870638, 0.931872, -0.953884, 0.903042, 0.760078, 0.209727, -0.285384, -0.45514, 0.113194, 0.0756611, 0.0924435, -0.472863, 0.960609, -0.160385, -0.839445, 0.457097, 0.163348, 0.344867, -0.131619, 0.688715, -0.540827, 0.571259, -0.95587, 0.506164, -0.155839, 0.0789621, 0.756772, -0.662069, 0.242908, 0.460821, 0.177872, -0.289839, -0.640603, 0.702598, -0.506406, -0.568262, -0.0713716, 0.413792, 0.159673, -0.305208, 0.133816, -0.160254, 0.787323, -0.753244, 0.600721, 0.263186, -0.162387, 0.477962, -0.702951, -0.731036, -0.939481, -0.524519, 0.934072, -0.511637, -0.503499, 0.106236, -0.323684, 0.534444, -0.843745, 0.364171, 0.0370358, -0.168801, -0.404559, -0.814178, 0.91745, -0.334276, 0.66925, -0.801201, 0.156511, -0.427949, 0.379153, 0.818597, -0.649902, 0.427087, -0.586015, -0.559789, -0.833923, 0.0892409, -0.621251, 0.213826, 0.465509, 0.4704, 0.380261, 0.413067, 0.180822, 0.172866, 0.59614, 0.825575, 0.662916, -0.704381, -0.297631, 0.697778],
    o6: [1.72003, 1.55816, 0.667546, 2.23663, 0.0661516, 0.290254, 0.770222, -1.58197, -0.850595, -0.484224, 0.949967, -0.577263, -0.871949, 2.34132, -0.135965, -0.985713, 0.815147, 1.03114, -1.41915, -0.515534, -0.373639, -1.50604, 0.673113, 3.06139, -0.388578, -1.76707, -0.315667, -1.03815, 0.432787, -1.41643, 1.12944, -0.175806, -0.846415, 1.40095, 0.70832, 2.19562, -2.61266, -0.705383, 1.26124, 1.46545, -2.35761, 2.04494, ]
}, {
    i5: [-0.295335, -0.00387601, -0.552251, 0.166084, -0.28482, -0.152143, -0.719885, -0.869386, -0.745598, 0.823947, 0.473183, -0.331337, 0.187631, 0.0426571, -0.826897, -0.755085, -0.472453, -0.0233656, 0.0483436, 0.933418, -0.961974, 0.0125783, 0.219742, 0.342604, -0.15166, 0.0934905, 0.783221, 0.129664, 0.838844, -0.271388, 0.924519, 0.342843, 0.274418, 0.350817, 0.841638, -0.543993, -0.00283395, -0.128467, -0.682943, -0.319117, 0.84634, 0.283003, 0.32865, 0.0293755, -0.0335696, 0.591266, -0.0743476, -0.741271, 0.462056, -0.583625, -0.590183, 0.6234, 0.535269, -0.670818, -0.955642, -0.770173, 0.479986, 0.664377, 0.399445, -0.968874, -0.276263, -0.901951, 0.544104, -0.958981, 0.482658, -0.807284, 0.305369, -0.947818, 0.827498, -0.382887, -0.805741, -0.796678, -0.299804, -0.229828, 0.818783, -0.103055, -0.45568, -0.227827, 0.543743, -0.96073, 0.946747, -0.857182, -0.96426, -0.292411, -0.715614, 0.765278, -0.475043, -0.590142, -0.238507, 0.673002, -0.473357, -0.319626, 0.936014, 0.486607, 0.580844, 0.425352, -0.800994, 0.290763, -0.494953, -0.441162, 0.718677, -0.828427, 0.96965, 7.53637e-05, -0.699973, -0.526886, -0.352682, 0.799466, 0.332789, 0.723389, 0.407659, -0.934084, -0.284705, 0.961484, -0.700395, -0.985808, -0.595342, -0.691721, 0.49448, -0.0842649, 0.0390966, 0.298938, -0.128094, -0.97158, 0.86393, 0.270606, -0.468986, -0.256605, 0.47215, -0.273117, -0.590343, -0.826529, -0.725381, -0.194821, -0.259661, -0.0949207, -0.180302, 0.0446834, -0.222133, -0.40393, 0.295772, -0.92949, 0.580079, -0.169856, 0.330311, 0.0173551, -0.635823, 0.475942, 0.907175, 0.242777, -0.512208, 0.362463, 0.0496289, 0.65171, 0.990057, 0.690733, -0.469013, -0.101311, -0.68372, -0.157841, -0.677711, -0.708224, -0.659437, -0.407607, 0.677033, 0.89032, 0.228307, -0.749514, 0.772958, 0.054701, 0.551705, 0.917052, -0.895022, -0.702397, 0.484142, 0.108648, 0.833347, 0.478872, -0.984112, 0.387176, -0.73299, 0.7526, 0.443312, -0.0987856, 0.125415, 0.10876, -0.498108, 0.43209, 0.344609, 0.928941, -0.130732, -0.0569167],
    o6: [1.28735, 1.91315, 2.51734, 0.375841, 0.637563, 2.653, 2.72959, 1.17389, -2.12119, 2.91417, -2.24246, 0.0497045, -0.127107, -0.144473, -0.393284, -2.02346, -0.239178, -0.246508, 1.29277, 1.32963, 0.117521, 0.0665713, 1.09438, -1.31426, 2.52594, -0.969211, 0.515478, -1.60926, 0.135211, 0.786415, -1.14382, -0.739102, -1.01731, 0.281615, 2.36311, 1.93872, -0.150491, 3.45217, 2.28219, 1.18282, -2.25086, 3.05468]
}, model=model_1_valid).AddNchw(i5, o6, layout).AddVariations("relaxed", "float16")


# TEST 7/8: CONV_3_H3_W2_[SAME|VALID]
i7 = Input("op1", "TENSOR_FLOAT32", "{1, 8, 8, 3}")
f7 = Parameter("op2", "TENSOR_FLOAT32", "{3, 3, 2, 3}", [-0.966213, -0.579455, -0.684259, 0.738216, 0.184325, 0.0973683, -0.176863, -0.23936, -0.000233404, 0.055546, -0.232658, -0.316404, -0.012904, 0.320705, -0.326657, -0.919674, 0.868081, -0.824608, -0.467474, 0.0278809, 0.563238, 0.386045, -0.270568, -0.941308, -0.779227, -0.261492, -0.774804, -0.79665, 0.22473, -0.414312, 0.685897, -0.327792, 0.77395, -0.714578, -0.972365, 0.0696099, -0.82203, -0.79946, 0.37289, -0.917775, 0.82236, -0.144706, -0.167188, 0.268062, 0.702641, -0.412223, 0.755759, 0.721547, -0.43637, -0.274905, -0.269165, 0.16102, 0.819857, -0.312008])
b7 = Parameter("op3", "TENSOR_FLOAT32", "{3}", [0., 0., 0.])
o7 = Output("op4", "TENSOR_FLOAT32", "{1, 8, 8, 3}")
o8 = Output("op4", "TENSOR_FLOAT32", "{1, 6, 7, 3}")
model_3_same = Model("3_H3_W2_SAME").Operation("CONV_2D", i7, f7, b7, 1, 1, 1, 0, layout).To(o7)
model_3_valid = Model("3_H3_W2_VALID").Operation("CONV_2D", i7, f7, b7, 2, 1, 1, 0, layout).To(o8)

example = Example({
    i7: [-0.869931, 0.644628, -0.918393, 0.153672, 0.868562, -0.358177, -0.134931, -0.247565, 0.22174, -0.259157, -0.284296, -0.538065, 0.765559, 0.41986, -0.556241, 0.658494, 0.214355, -0.850169, -0.252893, -0.478935, 0.530526, -0.0700663, -0.988729, -0.303061, 0.150845, 0.829915, 0.476349, 0.406537, -0.355343, 0.757145, -0.356362, 0.800482, -0.713861, 0.210483, -0.634303, 0.718236, -0.752038, 0.457547, -0.550769, -0.551178, 0.446766, -0.227462, 0.216348, -0.852806, -0.351486, 0.55906, -0.668493, -0.303493, -0.363763, -0.162837, 0.0701012, 0.756097, -0.142269, 0.329724, -0.656317, -0.998086, -0.652949, -0.40316, -0.893682, 0.432744, 0.612362, -0.869588, -0.71327, -0.398092, -0.0423559, 0.436576, -0.925272, 0.176549, 0.822904, 0.096833, -0.296802, -0.427195, 0.031654, -0.254479, 0.244905, 0.0948254, 0.643769, -0.90391, 0.352665, -0.901179, 0.266159, -0.968068, -0.615401, -0.388975, 0.939052, -0.116289, 0.107523, -0.0582711, 0.435172, 0.334675, 0.459711, 0.717436, 0.496627, -0.680175, -0.415066, 0.339848, 0.506004, -0.337808, -0.107218, -0.172496, 0.870638, 0.931872, -0.953884, 0.903042, 0.760078, 0.209727, -0.285384, -0.45514, 0.113194, 0.0756611, 0.0924435, -0.472863, 0.960609, -0.160385, -0.839445, 0.457097, 0.163348, 0.344867, -0.131619, 0.688715, -0.540827, 0.571259, -0.95587, 0.506164, -0.155839, 0.0789621, 0.756772, -0.662069, 0.242908, 0.460821, 0.177872, -0.289839, -0.640603, 0.702598, -0.506406, -0.568262, -0.0713716, 0.413792, 0.159673, -0.305208, 0.133816, -0.160254, 0.787323, -0.753244, 0.600721, 0.263186, -0.162387, 0.477962, -0.702951, -0.731036, -0.939481, -0.524519, 0.934072, -0.511637, -0.503499, 0.106236, -0.323684, 0.534444, -0.843745, 0.364171, 0.0370358, -0.168801, -0.404559, -0.814178, 0.91745, -0.334276, 0.66925, -0.801201, 0.156511, -0.427949, 0.379153, 0.818597, -0.649902, 0.427087, -0.586015, -0.559789, -0.833923, 0.0892409, -0.621251, 0.213826, 0.465509, 0.4704, 0.380261, 0.413067, 0.180822, 0.172866, 0.59614, 0.825575, 0.662916, -0.704381, -0.297631, 0.697778],
    o7: [-1.27853, 1.74987, -0.876718, 0.989692, 0.298548, 0.522103, -0.536896, -0.179382, -0.966914, 1.33708, 1.37042, -0.495494, 1.43859, -1.548, -0.430026, -0.662793, -0.0867897, -0.900658, -0.524396, 0.255731, -0.779081, 0.12666, 0.915651, -0.444765, -0.186842, -1.87308, 1.21135, -0.385009, 1.72032, -1.56036, -1.23059, 1.23694, 0.00200015, 0.359522, 1.60084, 0.434006, -0.282945, 2.37292, -1.28653, 0.0847837, -0.352093, -2.39659, 0.149246, 0.920351, -1.34346, 0.952311, -0.35811, 0.403449, 0.484796, -1.19989, -0.684298, -1.41301, 0.103177, -0.307039, 1.17741, 2.58936, -2.76237, -1.21565, -1.09619, 1.17432, 0.512143, 0.771379, 0.399879, -0.0533093, 0.290864, 0.95563, 1.16328, 1.80768, -1.52564, -0.126476, -0.185224, -0.114779, 1.2248, 0.237127, -0.213297, -0.619941, 0.497944, -1.68688, 1.59314, -0.127337, 0.111419, 1.13719, 1.68537, -0.479644, 1.18608, -2.52744, 1.34136, 0.548297, -2.0838, 2.64585, -0.993354, 0.128238, 1.26092, 0.318668, 0.893795, -0.0600559, -0.629126, -0.949229, 2.25828, -1.961, 0.00589599, -0.187854, -1.02403, 0.396121, 1.3704, 3.99355, 0.434221, 0.274464, -0.562438, -0.914871, 0.539129, -0.928687, 0.834954, 0.844178, -0.566053, -0.957341, 0.933336, 1.13613, -1.22109, 1.4649, -0.414666, -0.452821, -0.706006, -1.72657, -0.726574, -0.0979362, -0.478669, 1.78703, -0.639288, 1.48565, -0.179904, 1.01003, -0.317118, -0.675387, 1.90969, -1.38343, 0.697255, -0.292255, 1.81634, 0.717801, 0.862479, -0.407478, -0.343106, -0.0353232, -0.481893, -0.135565, -2.95941, 0.247846, 2.67757, -2.23999, -0.519673, 0.254447, 0.415283, -1.01065, 0.507911, 0.979926, -0.184304, -0.000950437, -0.734348, -0.196685, -0.713241, 0.594972, 0.0845042, 2.48496, 0.385019, -0.201145, 0.533332, -0.904872, -0.333518, -0.581063, -2.07065, 0.118687, -1.86708, -0.601987, 0.432037, 1.73923, 0.590007, 0.419788, 0.314198, 2.12817, 0.570793, -1.15998, -0.348587, -1.10231, -2.13091, 0.134467, -0.460382, 0.138338, 3.455, 0.679068, -0.190282, -0.0307461]
}, {
    i7: [-0.295335, -0.00387601, -0.552251, 0.166084, -0.28482, -0.152143, -0.719885, -0.869386, -0.745598, 0.823947, 0.473183, -0.331337, 0.187631, 0.0426571, -0.826897, -0.755085, -0.472453, -0.0233656, 0.0483436, 0.933418, -0.961974, 0.0125783, 0.219742, 0.342604, -0.15166, 0.0934905, 0.783221, 0.129664, 0.838844, -0.271388, 0.924519, 0.342843, 0.274418, 0.350817, 0.841638, -0.543993, -0.00283395, -0.128467, -0.682943, -0.319117, 0.84634, 0.283003, 0.32865, 0.0293755, -0.0335696, 0.591266, -0.0743476, -0.741271, 0.462056, -0.583625, -0.590183, 0.6234, 0.535269, -0.670818, -0.955642, -0.770173, 0.479986, 0.664377, 0.399445, -0.968874, -0.276263, -0.901951, 0.544104, -0.958981, 0.482658, -0.807284, 0.305369, -0.947818, 0.827498, -0.382887, -0.805741, -0.796678, -0.299804, -0.229828, 0.818783, -0.103055, -0.45568, -0.227827, 0.543743, -0.96073, 0.946747, -0.857182, -0.96426, -0.292411, -0.715614, 0.765278, -0.475043, -0.590142, -0.238507, 0.673002, -0.473357, -0.319626, 0.936014, 0.486607, 0.580844, 0.425352, -0.800994, 0.290763, -0.494953, -0.441162, 0.718677, -0.828427, 0.96965, 7.53637e-05, -0.699973, -0.526886, -0.352682, 0.799466, 0.332789, 0.723389, 0.407659, -0.934084, -0.284705, 0.961484, -0.700395, -0.985808, -0.595342, -0.691721, 0.49448, -0.0842649, 0.0390966, 0.298938, -0.128094, -0.97158, 0.86393, 0.270606, -0.468986, -0.256605, 0.47215, -0.273117, -0.590343, -0.826529, -0.725381, -0.194821, -0.259661, -0.0949207, -0.180302, 0.0446834, -0.222133, -0.40393, 0.295772, -0.92949, 0.580079, -0.169856, 0.330311, 0.0173551, -0.635823, 0.475942, 0.907175, 0.242777, -0.512208, 0.362463, 0.0496289, 0.65171, 0.990057, 0.690733, -0.469013, -0.101311, -0.68372, -0.157841, -0.677711, -0.708224, -0.659437, -0.407607, 0.677033, 0.89032, 0.228307, -0.749514, 0.772958, 0.054701, 0.551705, 0.917052, -0.895022, -0.702397, 0.484142, 0.108648, 0.833347, 0.478872, -0.984112, 0.387176, -0.73299, 0.7526, 0.443312, -0.0987856, 0.125415, 0.10876, -0.498108, 0.43209, 0.344609, 0.928941, -0.130732, -0.0569167],
    o7: [0.78574, 0.0700466, -0.110245, 0.0141003, -0.621007, -0.979104, 1.24104, 0.580398, -0.512997, 0.900559, -0.683229, -1.0162, 1.0089, -0.0752488, 0.110969, 0.270558, 0.756819, -0.10753, -0.371484, 0.149005, 0.0973829, 0.155766, -0.476502, 0.259481, 1.06709, -1.16534, 1.52694, -0.797245, 0.802736, -0.997109, 2.2661, -1.45548, 2.15506, -1.33682, 1.15225, -3.09324, 0.943457, 0.885211, 0.987944, -0.345875, -0.114708, 1.7107, 0.104745, 0.828324, -2.49964, -0.453742, -0.288829, -0.0948694, -0.489415, 1.74889, -0.378257, -2.10237, 0.613022, -2.5225, -0.746785, 3.63816, -1.9287, 0.774279, -0.613917, -0.650011, 1.03753, -0.177923, 0.891815, -1.00373, 1.83859, -1.59239, -0.0662623, 0.218806, -1.088, 0.280837, 0.902901, -1.90127, 3.04734, -1.57302, 1.10881, -0.980369, -3.85305, -0.955859, 1.64909, 2.33573, 0.31144, -0.594375, 0.325747, -0.952566, -0.613449, 2.85073, 1.94692, 1.12977, 1.1351, -0.449652, 0.118765, -0.199547, 2.873, 1.35182, -1.85457, 1.22364, 1.38049, 2.38342, 0.882321, 1.03795, -0.321571, -2.60202, -1.6372, 1.09302, 0.461768, 1.8485, -0.158928, 4.28871, -0.437375, -1.5794, 1.59869, 0.0811864, 0.912054, 0.452176, 2.01812, 2.62907, 1.50304, -0.840276, -0.455854, -0.224913, 0.609824, -0.11105, 3.35635, 2.02386, 1.4687, -0.708365, -0.508992, -3.02602, -0.75725, 1.85277, 2.92817, -0.172997, -1.13279, -0.355636, -0.337669, -0.588752, 2.05759, 1.0651, 0.884758, -0.0712112, 3.81319, 0.771629, 0.949634, 0.0838967, -2.19264, 0.114521, 0.543556, -1.63197, -0.267442, 1.15701, -2.37862, 2.57646, 0.531208, 0.9499, -0.231441, 1.51461, 1.58888, 0.895931, -0.753084, 0.545251, 0.746903, 0.012994, -0.790398, -1.1055, 1.77789, 0.430923, 0.818241, -0.731412, 0.979546, -2.48707, -1.53658, -1.66798, -1.04585, -0.667911, 1.00299, -2.20339, 0.137826, -2.31281, 0.755535, 0.495396, 0.549629, 0.713128, 0.751369, 0.283996, -0.814532, 1.4866, 1.12105, 0.927998, 0.517938, -0.612661, -1.47756, -1.42422]
}, model=model_3_same).AddNchw(i7, o7, layout).AddVariations("relaxed", "float16")

example = Example({
    i7: [-0.869931, 0.644628, -0.918393, 0.153672, 0.868562, -0.358177, -0.134931, -0.247565, 0.22174, -0.259157, -0.284296, -0.538065, 0.765559, 0.41986, -0.556241, 0.658494, 0.214355, -0.850169, -0.252893, -0.478935, 0.530526, -0.0700663, -0.988729, -0.303061, 0.150845, 0.829915, 0.476349, 0.406537, -0.355343, 0.757145, -0.356362, 0.800482, -0.713861, 0.210483, -0.634303, 0.718236, -0.752038, 0.457547, -0.550769, -0.551178, 0.446766, -0.227462, 0.216348, -0.852806, -0.351486, 0.55906, -0.668493, -0.303493, -0.363763, -0.162837, 0.0701012, 0.756097, -0.142269, 0.329724, -0.656317, -0.998086, -0.652949, -0.40316, -0.893682, 0.432744, 0.612362, -0.869588, -0.71327, -0.398092, -0.0423559, 0.436576, -0.925272, 0.176549, 0.822904, 0.096833, -0.296802, -0.427195, 0.031654, -0.254479, 0.244905, 0.0948254, 0.643769, -0.90391, 0.352665, -0.901179, 0.266159, -0.968068, -0.615401, -0.388975, 0.939052, -0.116289, 0.107523, -0.0582711, 0.435172, 0.334675, 0.459711, 0.717436, 0.496627, -0.680175, -0.415066, 0.339848, 0.506004, -0.337808, -0.107218, -0.172496, 0.870638, 0.931872, -0.953884, 0.903042, 0.760078, 0.209727, -0.285384, -0.45514, 0.113194, 0.0756611, 0.0924435, -0.472863, 0.960609, -0.160385, -0.839445, 0.457097, 0.163348, 0.344867, -0.131619, 0.688715, -0.540827, 0.571259, -0.95587, 0.506164, -0.155839, 0.0789621, 0.756772, -0.662069, 0.242908, 0.460821, 0.177872, -0.289839, -0.640603, 0.702598, -0.506406, -0.568262, -0.0713716, 0.413792, 0.159673, -0.305208, 0.133816, -0.160254, 0.787323, -0.753244, 0.600721, 0.263186, -0.162387, 0.477962, -0.702951, -0.731036, -0.939481, -0.524519, 0.934072, -0.511637, -0.503499, 0.106236, -0.323684, 0.534444, -0.843745, 0.364171, 0.0370358, -0.168801, -0.404559, -0.814178, 0.91745, -0.334276, 0.66925, -0.801201, 0.156511, -0.427949, 0.379153, 0.818597, -0.649902, 0.427087, -0.586015, -0.559789, -0.833923, 0.0892409, -0.621251, 0.213826, 0.465509, 0.4704, 0.380261, 0.413067, 0.180822, 0.172866, 0.59614, 0.825575, 0.662916, -0.704381, -0.297631, 0.697778],
    o8: [-0.186842, -1.87308, 1.21135, -0.385009, 1.72032, -1.56036, -1.23059, 1.23694, 0.00200015, 0.359522, 1.60084, 0.434006, -0.282945, 2.37292, -1.28653, 0.0847837, -0.352093, -2.39659, 0.149246, 0.920351, -1.34346, 0.484796, -1.19989, -0.684298, -1.41301, 0.103177, -0.307039, 1.17741, 2.58936, -2.76237, -1.21565, -1.09619, 1.17432, 0.512143, 0.771379, 0.399879, -0.0533093, 0.290864, 0.95563, 1.16328, 1.80768, -1.52564, 1.2248, 0.237127, -0.213297, -0.619941, 0.497944, -1.68688, 1.59314, -0.127337, 0.111419, 1.13719, 1.68537, -0.479644, 1.18608, -2.52744, 1.34136, 0.548297, -2.0838, 2.64585, -0.993354, 0.128238, 1.26092, -0.629126, -0.949229, 2.25828, -1.961, 0.00589599, -0.187854, -1.02403, 0.396121, 1.3704, 3.99355, 0.434221, 0.274464, -0.562438, -0.914871, 0.539129, -0.928687, 0.834954, 0.844178, -0.566053, -0.957341, 0.933336, -0.414666, -0.452821, -0.706006, -1.72657, -0.726574, -0.0979362, -0.478669, 1.78703, -0.639288, 1.48565, -0.179904, 1.01003, -0.317118, -0.675387, 1.90969, -1.38343, 0.697255, -0.292255, 1.81634, 0.717801, 0.862479, -0.481893, -0.135565, -2.95941, 0.247846, 2.67757, -2.23999, -0.519673, 0.254447, 0.415283, -1.01065, 0.507911, 0.979926, -0.184304, -0.000950437, -0.734348, -0.196685, -0.713241, 0.594972, 0.0845044, 2.48496, 0.385019]
}, {
    i7: [-0.295335, -0.00387601, -0.552251, 0.166084, -0.28482, -0.152143, -0.719885, -0.869386, -0.745598, 0.823947, 0.473183, -0.331337, 0.187631, 0.0426571, -0.826897, -0.755085, -0.472453, -0.0233656, 0.0483436, 0.933418, -0.961974, 0.0125783, 0.219742, 0.342604, -0.15166, 0.0934905, 0.783221, 0.129664, 0.838844, -0.271388, 0.924519, 0.342843, 0.274418, 0.350817, 0.841638, -0.543993, -0.00283395, -0.128467, -0.682943, -0.319117, 0.84634, 0.283003, 0.32865, 0.0293755, -0.0335696, 0.591266, -0.0743476, -0.741271, 0.462056, -0.583625, -0.590183, 0.6234, 0.535269, -0.670818, -0.955642, -0.770173, 0.479986, 0.664377, 0.399445, -0.968874, -0.276263, -0.901951, 0.544104, -0.958981, 0.482658, -0.807284, 0.305369, -0.947818, 0.827498, -0.382887, -0.805741, -0.796678, -0.299804, -0.229828, 0.818783, -0.103055, -0.45568, -0.227827, 0.543743, -0.96073, 0.946747, -0.857182, -0.96426, -0.292411, -0.715614, 0.765278, -0.475043, -0.590142, -0.238507, 0.673002, -0.473357, -0.319626, 0.936014, 0.486607, 0.580844, 0.425352, -0.800994, 0.290763, -0.494953, -0.441162, 0.718677, -0.828427, 0.96965, 7.53637e-05, -0.699973, -0.526886, -0.352682, 0.799466, 0.332789, 0.723389, 0.407659, -0.934084, -0.284705, 0.961484, -0.700395, -0.985808, -0.595342, -0.691721, 0.49448, -0.0842649, 0.0390966, 0.298938, -0.128094, -0.97158, 0.86393, 0.270606, -0.468986, -0.256605, 0.47215, -0.273117, -0.590343, -0.826529, -0.725381, -0.194821, -0.259661, -0.0949207, -0.180302, 0.0446834, -0.222133, -0.40393, 0.295772, -0.92949, 0.580079, -0.169856, 0.330311, 0.0173551, -0.635823, 0.475942, 0.907175, 0.242777, -0.512208, 0.362463, 0.0496289, 0.65171, 0.990057, 0.690733, -0.469013, -0.101311, -0.68372, -0.157841, -0.677711, -0.708224, -0.659437, -0.407607, 0.677033, 0.89032, 0.228307, -0.749514, 0.772958, 0.054701, 0.551705, 0.917052, -0.895022, -0.702397, 0.484142, 0.108648, 0.833347, 0.478872, -0.984112, 0.387176, -0.73299, 0.7526, 0.443312, -0.0987856, 0.125415, 0.10876, -0.498108, 0.43209, 0.344609, 0.928941, -0.130732, -0.0569167],
    o8: [1.06709, -1.16534, 1.52694, -0.797245, 0.802736, -0.997109, 2.2661, -1.45548, 2.15506, -1.33682, 1.15225, -3.09324, 0.943457, 0.885211, 0.987944, -0.345875, -0.114708, 1.7107, 0.104745, 0.828324, -2.49964, -0.489415, 1.74889, -0.378257, -2.10237, 0.613022, -2.5225, -0.746785, 3.63816, -1.9287, 0.774279, -0.613917, -0.650011, 1.03753, -0.177923, 0.891815, -1.00373, 1.83859, -1.59239, -0.0662623, 0.218806, -1.088, 3.04734, -1.57302, 1.10881, -0.980369, -3.85305, -0.955859, 1.64909, 2.33573, 0.31144, -0.594375, 0.325747, -0.952566, -0.613449, 2.85073, 1.94692, 1.12977, 1.1351, -0.449652, 0.118765, -0.199547, 2.873, 1.38049, 2.38342, 0.882321, 1.03795, -0.321571, -2.60202, -1.6372, 1.09302, 0.461768, 1.8485, -0.158928, 4.28871, -0.437375, -1.5794, 1.59869, 0.0811864, 0.912054, 0.452176, 2.01812, 2.62907, 1.50304, 0.609824, -0.11105, 3.35635, 2.02386, 1.4687, -0.708365, -0.508992, -3.02602, -0.75725, 1.85277, 2.92817, -0.172997, -1.13279, -0.355636, -0.337669, -0.588752, 2.05759, 1.0651, 0.884758, -0.0712112, 3.81319, -2.19264, 0.114521, 0.543556, -1.63197, -0.267442, 1.15701, -2.37862, 2.57646, 0.531208, 0.9499, -0.231441, 1.51461, 1.58888, 0.895931, -0.753084, 0.545251, 0.746904, 0.0129939, -0.790398, -1.1055, 1.77789]
}, model=model_3_valid).AddNchw(i7, o8, layout).AddVariations("relaxed", "float16")

# TEST 9: quantized with scale product greater than output scale
scale = 256.5 / 255
zero_point = 128
i9 = Input("op1", ("TENSOR_QUANT8_ASYMM", [2, 2, 4, 1], scale, zero_point))
f9 = Parameter("op2", ("TENSOR_QUANT8_ASYMM", [3, 2, 2, 1], scale, zero_point),
               [129, 130, 131, 132, 127, 129, 127, 129, 127, 127, 129, 129])
b9 = Parameter("op3", ("TENSOR_INT32", [3], scale * scale, 0), [1, 2, 3])
o9 = Output("op4", ("TENSOR_QUANT8_ASYMM", [2, 1, 2, 3], 1.0, 127))
model9 = Model().Operation("CONV_2D", i9, f9, b9, 2, 2, 2, 0).To(o9)

# Instantiate an example
example = Example({
    i9: [
        129, 129, 129, 129, 130, 130, 130, 130, 129, 130, 131, 132, 129, 130,
        131, 132
    ],
    o9: [145, 129, 132, 145, 129, 132, 144, 131, 130, 164, 131, 130]
}, model=model9, name="quant_output_multiplier_gt_1").AddInput(f9, b9).AddVariations("relaxed")
