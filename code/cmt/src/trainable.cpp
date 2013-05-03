#include <sys/time.h>
#include <cstdlib>
#include "trainable.h"
#include "exception.h"

#include <limits>
using std::numeric_limits;

#include <iostream>
using std::cout;
using std::endl;

#include <iomanip>
using std::setw;
using std::setprecision;

CMT::Trainable::Callback::~Callback() {
}



CMT::Trainable::Parameters::Parameters() {
	verbosity = 0;
	maxIter = 1000;
	threshold = 1e-5;
	numGrad = 20;
	batchSize = 2000;
	callback = 0;
	cbIter = 25;
	valIter = 5;
	valLookAhead = 0;
}



CMT::Trainable::Parameters::~Parameters() {
	if(callback)
		delete callback;
}



CMT::Trainable::Parameters::Parameters(const Parameters& params) :
	verbosity(params.verbosity),
	maxIter(params.maxIter),
	threshold(params.threshold),
	numGrad(params.numGrad),
	batchSize(params.batchSize),
	callback(0),
	cbIter(params.cbIter),
	valIter(params.valIter),
	valLookAhead(params.valLookAhead)
{
	if(params.callback)
		callback = params.callback->copy();
}



CMT::Trainable::Parameters& CMT::Trainable::Parameters::operator=(
	const Parameters& params)
{
	verbosity = params.verbosity;
	maxIter = params.maxIter;
	threshold = params.threshold;
	numGrad = params.numGrad;
	batchSize = params.batchSize;
	callback = params.callback ? params.callback->copy() : 0;
	cbIter = params.cbIter;
	valIter = params.valIter;
	valLookAhead = params.valLookAhead;

	return *this;
}



CMT::Trainable::InstanceLBFGS::InstanceLBFGS(
	CMT::Trainable* cd,
	const CMT::Trainable::Parameters* params,
	const MatrixXd* input,
	const MatrixXd* output) :
	cd(cd),
	params(params),
	input(input),
	output(output),
	inputVal(0),
	outputVal(0),
	logLoss(numeric_limits<double>::max()),
	counter(0),
	parameters(0)
{
}



CMT::Trainable::InstanceLBFGS::InstanceLBFGS(
	CMT::Trainable* cd,
	const CMT::Trainable::Parameters* params,
	const MatrixXd* input,
	const MatrixXd* output,
	const MatrixXd* inputVal,
	const MatrixXd* outputVal) :
	cd(cd),
	params(params),
	input(input),
	output(output),
	inputVal(inputVal),
	outputVal(outputVal),
	logLoss(numeric_limits<double>::max()),
	counter(0),
	parameters(cd->parameters(*params))
{
}



CMT::Trainable::InstanceLBFGS::~InstanceLBFGS() {
	if(parameters)
		lbfgs_free(parameters);
}



CMT::Trainable::~Trainable() {
}



int CMT::Trainable::callbackLBFGS(
	void* instance,
	const lbfgsfloatval_t* x,
	const lbfgsfloatval_t* g,
	const lbfgsfloatval_t fx,
	const lbfgsfloatval_t xnorm,
	const lbfgsfloatval_t gnorm,
	const lbfgsfloatval_t, int,
	int iteration,
	int)
{
	// unpack user data
	InstanceLBFGS* inst = static_cast<InstanceLBFGS*>(instance);

	const CMT::Trainable::Parameters& params = *inst->params;

	if(inst->inputVal && inst->outputVal && iteration % params.valIter == 0) {
		inst->cd->setParameters(x, params);

		double logLoss = inst->cd->evaluate(*inst->inputVal, *inst->outputVal);

		if(params.verbosity > 0) {
			cout << setw(6) << iteration;
			cout << setw(11) << setprecision(5) << fx;
			cout << setw(11) << setprecision(5) << logLoss << endl;
		}

		if(logLoss < inst->logLoss) {
			// store current parameters for later
			for(int i = 0, N = inst->cd->numParameters(params); i < N; ++i)
				inst->parameters[i] = x[i];

			inst->counter = 0;
			inst->logLoss = logLoss;
		} else {
			inst->counter += 1;

			if(params.valLookAhead > 0 && inst->counter >= params.valLookAhead)
				// performance did not improve for valLookAhead times
				return 1;
		}
	} else {
		if(params.verbosity > 0)
			cout << setw(6) << iteration << setw(11) << setprecision(5) << fx << endl;
	}

	if(params.callback && iteration % params.cbIter == 0) {
		inst->cd->setParameters(x, params);

		if(!(*params.callback)(iteration, *inst->cd))
			return 1;
	}

	return 0;
}



lbfgsfloatval_t CMT::Trainable::evaluateLBFGS(
	void* instance,
	const lbfgsfloatval_t* x,
	lbfgsfloatval_t* g,
	int, double)
{
	// unpack user data
	const InstanceLBFGS& inst = *static_cast<InstanceLBFGS*>(instance);
	const CMT::Trainable& cd = *inst.cd;
	const CMT::Trainable::Parameters& params = *inst.params;
	const MatrixXd& input = *inst.input;
	const MatrixXd& output = *inst.output;

	return cd.parameterGradient(input, output, x, g, params);
}



void CMT::Trainable::initialize(const MatrixXd& input, const MatrixXd& output) {
}



void CMT::Trainable::initialize(const pair<ArrayXXd, ArrayXXd>& data) {
	initialize(data.first, data.second);
}



bool CMT::Trainable::train(
	const MatrixXd& input,
	const MatrixXd& output,
	const Parameters& params)
{
	return train(input, output, 0, 0, params);
}



bool CMT::Trainable::train(
	const MatrixXd& input,
	const MatrixXd& output,
	const MatrixXd& inputVal,
	const MatrixXd& outputVal,
	const Parameters& params)
{
	return train(input, output, &inputVal, &outputVal, params);
}



bool CMT::Trainable::train(
	const pair<ArrayXXd, ArrayXXd>& data,
	const Parameters& params)
{
	return train(data.first, data.second, 0, 0, params);
}



bool CMT::Trainable::train(
	const pair<ArrayXXd, ArrayXXd>& data,
	const pair<ArrayXXd, ArrayXXd>& dataVal,
	const Parameters& params)
{
	return train(
		data.first,
		data.second,
		dataVal.first,
		dataVal.second,
		params);
}



bool CMT::Trainable::train(
	const MatrixXd& input,
	const MatrixXd& output,
	const MatrixXd* inputVal,
	const MatrixXd* outputVal,
	const Parameters& params)
{
	if(input.rows() != dimIn() || output.rows() != dimOut())
		throw Exception("Data has wrong dimensionality.");

	if(input.cols() != output.cols())
		throw Exception("The number of inputs and outputs should be the same.");

	// copy parameters for L-BFGS
	lbfgsfloatval_t* x = parameters(params);

	// optimization hyperparameters
	lbfgs_parameter_t hyperparams;
	lbfgs_parameter_init(&hyperparams);
	hyperparams.max_iterations = params.maxIter;
	hyperparams.m = params.numGrad;
	hyperparams.epsilon = params.threshold;
	hyperparams.linesearch = LBFGS_LINESEARCH_MORETHUENTE;
	hyperparams.max_linesearch = 100;
	hyperparams.ftol = 1e-4;
	hyperparams.xtol = 1e-32;

	// wrap additional arguments
	InstanceLBFGS instance(this, &params, &input, &output, inputVal, outputVal);

	if(params.verbosity > 0)
		if(inputVal && outputVal) {
			cout << setw(6) << 0;
			cout << setw(11) << setprecision(5) << evaluate(input, output);
			cout << setw(11) << setprecision(5) << evaluate(*inputVal, *outputVal) << endl;
		} else {
			cout << setw(6) << 0;
			cout << setw(11) << setprecision(5) << evaluate(input, output) << endl;
		}

	// start LBFGS optimization
	int status = LBFGSERR_MAXIMUMITERATION;
	if(params.maxIter > 0)
		status = lbfgs(numParameters(params), x, 0,
			&evaluateLBFGS,
			&callbackLBFGS,
			&instance,
			&hyperparams);

	// copy parameters back
	setParameters(x, params);

	if(inputVal && outputVal && instance.parameters) {
		double logLoss = evaluate(*inputVal, *outputVal);

		// use parameters which minimize the validation error
		setParameters(instance.parameters, params);

		// check that they really give a smaller validation error
		if(evaluate(*inputVal, *outputVal) > logLoss)
			// otherwise, use other parameters after all
			setParameters(x, params);
	}

	// free memory used by LBFGS
	lbfgs_free(x);

	if(status >= 0) {
		return true;
	} else {
		if(status != LBFGSERR_MAXIMUMITERATION)
			cout << "There might be a problem with the optimization (" << status << ")." << endl;
		return false;
	}
}



double CMT::Trainable::checkGradient(
	const MatrixXd& input,
	const MatrixXd& output,
	double epsilon,
	const Parameters& params)
{
	if(input.rows() != dimIn() || output.rows() != dimOut())
		throw Exception("Data has wrong dimensionality.");
	if(input.cols() != output.cols())
		throw Exception("The number of inputs and outputs should be the same.");

	// request memory for LBFGS and copy parameters
	lbfgsfloatval_t* x = parameters(params);

	int numParams = numParameters(params);

	lbfgsfloatval_t y[numParams];
	lbfgsfloatval_t g[numParams];
	lbfgsfloatval_t n[numParams];
	lbfgsfloatval_t val1;
	lbfgsfloatval_t val2;

	// make another copy
	for(int i = 0; i < numParams; ++i)
		y[i] = x[i];

	// arguments to LBFGS function
	InstanceLBFGS instance(this, &params, &input, &output);

	// compute numerical gradient using central differences
	for(int i = 0; i < numParams; ++i) {
		y[i] = x[i] + epsilon;
		val1 = evaluateLBFGS(&instance, y, 0, 0, 0.);
		y[i] = x[i] - epsilon;
		val2 = evaluateLBFGS(&instance, y, 0, 0, 0.);
		y[i] = x[i];
		n[i] = (val1 - val2) / (2. * epsilon);
	}

	// compute analytical gradient
	evaluateLBFGS(&instance, x, g, 0, 0.);

	// squared error
	double err = 0.;
	for(int i = 0; i < numParams; ++i)
		err += (g[i] - n[i]) * (g[i] - n[i]);

	return sqrt(err);
}



double CMT::Trainable::checkPerformance(
	const MatrixXd& input,
	const MatrixXd& output,
	int repetitions,
	const Parameters& params)
{
	if(input.rows() != dimIn() || output.rows() != dimOut())
		throw Exception("Data has wrong dimensionality.");
	if(input.cols() != output.cols())
		throw Exception("The number of inputs and outputs should be the same.");

	// request memory for LBFGS and copy parameters
	lbfgsfloatval_t* x = parameters(params);

	// optimization hyperparameters
	lbfgs_parameter_t paramsLBFGS;
	lbfgs_parameter_init(&paramsLBFGS);
	paramsLBFGS.max_iterations = params.maxIter;
	paramsLBFGS.m = params.numGrad;

	// wrap additional arguments
	InstanceLBFGS instance(this, &params, &input, &output);

	// measure time it takes to evaluate gradient
	lbfgsfloatval_t* g = lbfgs_malloc(numParameters(params));
	timeval from, to;

	gettimeofday(&from, 0);
	for(int i = 0; i < repetitions; ++i)
		evaluateLBFGS(&instance, x, g, 0, 0.);
	gettimeofday(&to, 0);

	// free memory used by LBFGS
	lbfgs_free(x);

	return (to.tv_sec + to.tv_usec / 1E6 - from.tv_sec - from.tv_usec / 1E6) / repetitions;
}
