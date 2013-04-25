import sys
import unittest

sys.path.append('./code')

from cmt import MCBM, PatchMCBM
from numpy import *
from numpy import max
from numpy.random import *
from pickle import dump, load
from tempfile import mkstemp

class Tests(unittest.TestCase):
	def test_basics(self):
		dim_in = 10
		num_components = 7
		num_features = 50
		num_samples = 100

		# create model
		mcbm = MCBM(dim_in, num_components, num_features)

		# generate output
		input = randint(2, size=[dim_in, num_samples])
		output = mcbm.sample(input)
		loglik = mcbm.loglikelihood(input, output)
		#post = mcbm.posterior(input, output)
		#samples = mcbm.sample_posterior(input, output)

		# check hyperparameters
		self.assertEqual(mcbm.dim_in, dim_in)
		self.assertEqual(mcbm.num_components, num_components)
		self.assertEqual(mcbm.num_features, num_features)
	
		# check parameters
		self.assertEqual(mcbm.priors.shape[0], num_components)
		self.assertEqual(mcbm.priors.shape[1], 1)
		self.assertEqual(mcbm.weights.shape[0], num_components)
		self.assertEqual(mcbm.weights.shape[1], num_features)
		self.assertEqual(mcbm.features.shape[0], dim_in)
		self.assertEqual(mcbm.features.shape[1], num_features)
		self.assertEqual(mcbm.predictors.shape[0], num_components)
		self.assertEqual(mcbm.predictors.shape[1], dim_in)
		self.assertEqual(mcbm.input_bias.shape[0], dim_in)
		self.assertEqual(mcbm.input_bias.shape[1], num_components)
		self.assertEqual(mcbm.output_bias.shape[0], num_components)
		self.assertEqual(mcbm.output_bias.shape[1], 1)

		# check dimensionality of output
		self.assertEqual(output.shape[0], 1)
		self.assertEqual(output.shape[1], num_samples)
		self.assertEqual(loglik.shape[0], 1)
		self.assertEqual(loglik.shape[1], num_samples)
		#self.assertEqual(post.shape[0], num_components)
		#self.assertEqual(post.shape[1], num_samples)
		#self.assertEqual(samples.shape[0], 1)
		#self.assertEqual(samples.shape[1], num_samples)



	def test_train(self):
		mcbm = MCBM(8, 4, 20)

		parameters = mcbm._parameters()

		mcbm.train(
			randint(2, size=[mcbm.dim_in, 2000]),
			randint(2, size=[mcbm.dim_out, 2000]),
			parameters={
				'verbosity': 0,
				'max_iter': 0,
				})

		# parameters should not have changed
		self.assertLess(max(abs(mcbm._parameters() - parameters)), 1e-20)

		def callback(i, mcbm):
			return

		mcbm.train(
			randint(2, size=[mcbm.dim_in, 10000]),
			randint(2, size=[mcbm.dim_out, 10000]),
			parameters={
				'verbosity': 0,
				'max_iter': 10,
				'threshold': 0.,
				'batch_size': 1999,
				'callback': callback,
				'cb_iter': 1,
				})



	def test_gradient(self):
		mcbm = MCBM(5, 2, 10)

		# choose random parameters
		mcbm._set_parameters(randn(*mcbm._parameters().shape))

		err = mcbm._check_gradient(
			randint(2, size=[mcbm.dim_in, 1000]),
			randint(2, size=[mcbm.dim_out, 1000]), 1e-5)
		self.assertLess(err, 1e-8)

		# test with regularization turned off
		for param in ['priors', 'weights', 'features', 'pred', 'input_bias', 'output_bias']:
			err = mcbm._check_gradient(
				randint(2, size=[mcbm.dim_in, 1000]),
				randint(2, size=[mcbm.dim_out, 1000]),
				1e-5,
				parameters={
					'train_prior': param == 'priors',
					'train_weights': param == 'weights',
					'train_features': param == 'features',
					'train_predictors': param == 'pred',
					'train_input_bias': param == 'input_bias',
					'train_output_bias': param == 'output_bias',
				})
			self.assertLess(err, 1e-8)

		# test with regularization turned on
		for param in ['priors', 'weights', 'features', 'pred', 'input_bias', 'output_bias']:
			err = mcbm._check_gradient(
				randint(2, size=[mcbm.dim_in, 1000]),
				randint(2, size=[mcbm.dim_out, 1000]),
				1e-5,
				parameters={
					'train_prior': param == 'priors',
					'train_weights': param == 'weights',
					'train_features': param == 'features',
					'train_predictors': param == 'pred',
					'train_input_bias': param == 'input_bias',
					'train_output_bias': param == 'output_bias',
					'regularize_features': True,
					'regularize_predictors': True,
				})
			self.assertLess(err, 1e-8)



	def test_pickle(self):
		mcbm0 = MCBM(11, 4, 21)

		tmp_file = mkstemp()[1]

		# store model
		with open(tmp_file, 'w') as handle:
			dump({'mcbm': mcbm0}, handle)

		# load model
		with open(tmp_file) as handle:
			mcbm1 = load(handle)['mcbm']

		# make sure parameters haven't changed
		self.assertEqual(mcbm0.dim_in, mcbm1.dim_in)
		self.assertEqual(mcbm0.num_components, mcbm1.num_components)
		self.assertEqual(mcbm0.num_features, mcbm1.num_features)

		self.assertLess(max(abs(mcbm0.priors - mcbm1.priors)), 1e-20)
		self.assertLess(max(abs(mcbm0.weights - mcbm1.weights)), 1e-20)
		self.assertLess(max(abs(mcbm0.features - mcbm1.features)), 1e-20)
		self.assertLess(max(abs(mcbm0.predictors - mcbm1.predictors)), 1e-20)
		self.assertLess(max(abs(mcbm0.input_bias - mcbm1.input_bias)), 1e-20)
		self.assertLess(max(abs(mcbm0.output_bias - mcbm1.output_bias)), 1e-20)



	def test_patchmcbm(self):
		xmask = ones([8, 8], dtype='bool')
		ymask = zeros([8, 8], dtype='bool')
		xmask[-1, -1] = False
		ymask[-1, -1] = True

		model = PatchMCBM(8, 8, xmask, ymask, MCBM(sum(xmask), 1))

		for i in range(8):
			for j in range(8):
				self.assertEqual(model[i, j].dim_in, (i + 1) * (j + 1) - 1)
				self.assertTrue(isinstance(model[i, j], MCBM))



	def test_patchmcbm_train(self):
		xmask = ones([2, 2], dtype='bool')
		ymask = zeros([2, 2], dtype='bool')
		xmask[-1, -1] = False
		ymask[-1, -1] = True

		model = PatchMCBM(2, 2, xmask, ymask, MCBM(sum(xmask), 1, 1))

		# checkerboard
		data = array([[0, 1], [1, 0]], dtype='bool').reshape(-1, 1)
		data = tile(data, (1, 1000)) ^ (randn(1, 1000) > .5)

		model.initialize(data, parameters={'max_iter': 100})

		# training should converge in much less than 2000 iterations
		self.assertTrue(model.train(data, parameters={'max_iter': 2000}))
		
		samples = model.sample(1000) > .5
		samples ^= samples[0]

		# less than 1 percent should have wrong pattern
		self.assertLess(mean(0 - samples[0]), 0.01)
		self.assertLess(mean(1 - samples[1]), 0.01)
		self.assertLess(mean(1 - samples[2]), 0.01)
		self.assertLess(mean(0 - samples[3]), 0.01)



	def test_patchmcbm_loglikelihood(self):
		xmask = ones([2, 2], dtype='bool')
		ymask = zeros([2, 2], dtype='bool')
		xmask[-1, -1] = False
		ymask[-1, -1] = True

		model = PatchMCBM(2, 2, xmask, ymask)

		samples = model.sample(100)

		self.assertFalse(isnan(mean(model.loglikelihood(samples))))



	def test_patchmcbm_subscript(self):
		xmask = ones([2, 2], dtype='bool')
		ymask = zeros([2, 2], dtype='bool')
		xmask[-1, -1] = False
		ymask[-1, -1] = True

		model = PatchMCBM(2, 2, xmask, ymask)

		mcbm = MCBM(model[0, 1].dim_in, 12, 47)

		model[0, 1] = mcbm

		self.assertEqual(model[0, 1].num_components, mcbm.num_components)
		self.assertEqual(model[0, 1].num_features, mcbm.num_features)

		def wrong_assign():
			model[1, 1] = 'string'

		self.assertRaises(TypeError, wrong_assign)



	def test_patchmcbm_pickle(self):
		xmask = ones([2, 2], dtype='bool')
		ymask = zeros([2, 2], dtype='bool')
		xmask[-1, -1] = False
		ymask[-1, -1] = True

		model0 = PatchMCBM(2, 2, xmask, ymask)

		tmp_file = mkstemp()[1]

		# store model
		with open(tmp_file, 'w') as handle:
			dump({'model': model0}, handle)

		# load model
		with open(tmp_file) as handle:
			model1 = load(handle)['model']

		# make sure parameters haven't changed
		self.assertEqual(model0.rows, model1.rows)
		self.assertEqual(model0.cols, model1.cols)
		self.assertLess(max(abs(model0[0, 0].priors - model1[0, 0].priors)), 1e-20)
		self.assertLess(max(abs(model0[1, 0].weights - model1[1, 0].weights)), 1e-20)
		self.assertLess(max(abs(model0[1, 0].features - model1[1, 0].features)), 1e-20)
		self.assertLess(max(abs(model0[0, 1].predictors - model1[0, 1].predictors)), 1e-20)
		self.assertLess(max(abs(model0[1, 1].input_bias - model1[1, 1].input_bias)), 1e-20)
		self.assertLess(max(abs(model0[1, 1].output_bias - model1[1, 1].output_bias)), 1e-20)



if __name__ == '__main__':
	unittest.main()
