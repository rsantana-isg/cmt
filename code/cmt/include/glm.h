#ifndef CMT_GLM_H
#define CMT_GLM_H

#include "Eigen/Core"
#include "trainable.h"
#include "distribution.h"

namespace CMT {
	using Eigen::Array;
	using Eigen::Dynamic;
	using Eigen::VectorXd;

	/**
	 * A generic class for generalized linear models.
	 */
	class GLM : public Trainable {
		public:
			class Nonlinearity {
				public:
					virtual Nonlinearity* copy() = 0;

					virtual Array<double, 1, Dynamic> operator()(
						const Array<double, 1, Dynamic>& data) const = 0;
					virtual Array<double, 1, Dynamic> derivative(
						const Array<double, 1, Dynamic>& data) const = 0;
			};

			class UnivariateDistribution : public Distribution {
				public:
					inline int dim() const;

					virtual UnivariateDistribution* copy() = 0;

					/**
					 * Log-likelihood for different settings of the mean parameter.
					 *
					 * @param data data points for which to evaluate log-likelihood
					 * @param means parameters for which to evaluate log-likelihood
					 */
					virtual Array<double, 1, Dynamic> logLikelihood(
						const Array<double, 1, Dynamic>& data,
						const Array<double, 1, Dynamic>& means) const = 0;

					virtual MatrixXd sample(
						const Array<double, 1, Dynamic>& means) const = 0;

					/**
					 * Derivative of the *negative* log-likelihood with respect to the mean.
					 *
					 * @param data data points for which to evaluate gradient
					 * @param means parameters for which to evaluate gradient
					 */
					virtual Array<double, 1, Dynamic> gradient(
						const Array<double, 1, Dynamic>& data,
						const Array<double, 1, Dynamic>& means) const = 0;
			};

			using ConditionalDistribution::logLikelihood;

			GLM(
				int dimIn,
				Nonlinearity* nonlinearity,
				UnivariateDistribution* distribution);
			GLM(const GLM& glm);
			virtual ~GLM();

			GLM& operator=(const GLM& glm);

			inline int dimIn() const;
			inline int dimOut() const;

			inline const Nonlinearity& nonlinearity() const;
			inline const UnivariateDistribution& distribution() const;

			inline VectorXd weights() const;
			inline void setWeights(const VectorXd& weights);

			virtual Array<double, 1, Dynamic> logLikelihood(
				const MatrixXd& input,
				const MatrixXd& output) const;

			virtual MatrixXd sample(const MatrixXd& input) const;

			virtual pair<pair<ArrayXXd, ArrayXXd>, Array<double, 1, Dynamic> > computeDataGradient(
				const MatrixXd& input,
				const MatrixXd& output) const;

		protected:
			int mDimIn;
			VectorXd mWeights;
			Nonlinearity* mNonlinearity;
			UnivariateDistribution* mDistribution;

			virtual int numParameters(const Parameters& params = Parameters()) const;
			virtual lbfgsfloatval_t* parameters(
				const Parameters& params = Parameters()) const;
			virtual void setParameters(
				const lbfgsfloatval_t* x,
				const Parameters& params = Parameters());

			virtual double parameterGradient(
				const MatrixXd& input,
				const MatrixXd& output,
				const lbfgsfloatval_t* x,
				lbfgsfloatval_t* g,
				const Parameters& params) const;
	};

	class LogisticFunction : public GLM::Nonlinearity {
		virtual LogisticFunction* copy();

		virtual Array<double, 1, Dynamic> operator()(
			const Array<double, 1, Dynamic>& data) const;
		virtual Array<double, 1, Dynamic> derivative(
			const Array<double, 1, Dynamic>& data) const;
	};

	class Bernoulli : public GLM::UnivariateDistribution {
		public:
			Bernoulli(double prob = 0.5);

			virtual Bernoulli* copy();

			virtual MatrixXd sample(int numSamples) const;
			virtual MatrixXd sample(
				const Array<double, 1, Dynamic>& means) const;

			virtual Array<double, 1, Dynamic> logLikelihood(
				const MatrixXd& data) const;
			virtual Array<double, 1, Dynamic> logLikelihood(
				const Array<double, 1, Dynamic>& data,
				const Array<double, 1, Dynamic>& means) const;

			virtual Array<double, 1, Dynamic> gradient(
				const Array<double, 1, Dynamic>& data,
				const Array<double, 1, Dynamic>& means) const;

		protected:
			double mProb;
	};
}



inline int CMT::GLM::UnivariateDistribution::dim() const {
	return 1;
}



inline int CMT::GLM::dimIn() const {
	return mDimIn;
}



inline int CMT::GLM::dimOut() const {
	return 1;
}



inline const CMT::GLM::Nonlinearity& CMT::GLM::nonlinearity() const {
	return *mNonlinearity;
}



inline const CMT::GLM::UnivariateDistribution& CMT::GLM::distribution() const {
	return *mDistribution;
}



inline Eigen::VectorXd CMT::GLM::weights() const {
	return mWeights;
}



inline void CMT::GLM::setWeights(const VectorXd& weights) {
	mWeights = weights;
}

#endif
