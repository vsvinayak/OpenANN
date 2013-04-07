#include <RBM.h>
#include <Random.h>
#include <ActivationFunctions.h>
#include <EigenWrapper.h>

namespace OpenANN {

RBM::RBM(int D, int H, int cdN, fpt stdDev)
  : D(D), H(H), cdN(cdN), stdDev(stdDev),
    W(H, D), posGradW(H, D), negGradW(H, D),
    bv(D), posGradBv(D), negGradBv(D),
    bh(H), posGradBh(H), negGradBh(H),
    pv(D), v(D), ph(H), h(H)
{
}

Vt RBM::operator()(const Vt& x)
{
}

bool RBM::providesInitialization()
{
  return true;
}

void RBM::initialize()
{
  RandomNumberGenerator rng;
  for(int j = 0; j < H; j++)
    for(int i = 0; i < D; i++)
      W(j, i) = rng.sampleNormalDistribution<fpt>() * stdDev;
  for(int i = 0; i < D; i++)
    bv(i) = rng.sampleNormalDistribution<fpt>() * stdDev;
  for(int j = 0; j < H; j++)
    bh(j) = rng.sampleNormalDistribution<fpt>() * stdDev;
}

unsigned int RBM::examples()
{
  return trainSet->samples();
}

unsigned int RBM::dimension()
{
  return D*H + D + H;
}

void RBM::setParameters(const Vt& parameters)
{
  int idx = 0;
  for(int j = 0; j < H; j++)
    for(int i = 0; i < D; i++)
      W(j, i) = parameters(idx++);
  for(int i = 0; i < D; i++)
    bv(i) = parameters(idx++);
  for(int j = 0; j < H; j++)
    bh(j) = parameters(idx++);
  OPENANN_CHECK_MATRIX_BROKEN(parameters);
}

Vt RBM::currentParameters()
{
  Vt parameters(dimension());
  int idx = 0;
  for(int j = 0; j < H; j++)
    for(int i = 0; i < D; i++)
      parameters(idx++) = W(j, i);
  for(int i = 0; i < D; i++)
    parameters(idx++) = bv(i);
  for(int j = 0; j < H; j++)
    parameters(idx++) = bh(j);
  OPENANN_CHECK_MATRIX_BROKEN(parameters);
  return parameters;
}

fpt RBM::error()
{
  return 0.0; // TODO reconstruction error?
}

bool RBM::providesGradient()
{
  return true;
}

Vt RBM::gradient()
{
  // TODO CD-n
}

Vt RBM::gradient(unsigned int i)
{
  reality(i);
  daydream();

  Vt gradient(dimension());
  int idx = 0;
  for(int j = 0; j < H; j++)
    for(int i = 0; i < D; i++)
      gradient(idx++) = posGradW(j, i) - negGradW(j, i);
  for(int i = 0; i < D; i++)
    gradient(idx++) = posGradBv(i) - negGradBv(i);
  for(int j = 0; j < H; j++)
    gradient(idx++) = posGradBh(j) - negGradBh(j);
  return -gradient;
}

bool RBM::providesHessian()
{
  return false;
}

Mt RBM::hessian()
{
  // TODO return dummy
}

Learner& RBM::trainingSet(Mt& trainingInput, Mt& trainingOutput)
{
  // TODO
}

Learner& RBM::trainingSet(DataSet& trainingSet)
{
  trainSet = &trainingSet;
}

void RBM::train(fpt alpha, int epochs)
{
  RandomNumberGenerator rng;
  for(int e = 0; e < epochs; e++)
  {
    for(int n = 0; n < trainSet->samples(); n++)
    {
      int idx = n;
      reality(idx);
      daydream();
      W += alpha * (posGradW - negGradW);
      bh += alpha * (posGradBh - negGradBh);
      bv += alpha * (posGradBv - negGradBv);
    }
  }
}

Vt RBM::reconstructProb(int n)
{
  v = trainSet->getInstance(n);
  sampleHgivenV();
  sampleVgivenH();
  return pv;
}

Vt RBM::reconstruct(int n)
{
  v = trainSet->getInstance(n);
  sampleHgivenV();
  sampleVgivenH();
  return v;
}

void RBM::reality(int n)
{
  v = trainSet->getInstance(n);

  sampleHgivenV();

  posGradW = ph * v.transpose();
  posGradBv = v;
  posGradBh = ph;
}

void RBM::daydream()
{
  for(int n = 0; n < cdN; n++)
  {
    sampleVgivenH();
    sampleHgivenV();
  }

  negGradW = ph * pv.transpose();
  negGradBv = pv;
  negGradBh = ph;
}

void RBM::sampleHgivenV()
{
  ph = W * v + bh;
  activationFunction(LOGISTIC, ph, ph);
  RandomNumberGenerator rng;
  for(int j = 0; j < H; j++)
    h(j) = (fpt) (ph(j) <= rng.generate<fpt>(0.0, 1.0));
}

void RBM::sampleVgivenH()
{
  pv = W.transpose() * h + bv;
  activationFunction(LOGISTIC, pv, pv);
  RandomNumberGenerator rng;
  for(int i = 0; i < D; i++)
    v(i) = (fpt) (pv(i) <= rng.generate<fpt>(0.0, 1.0));
}

}
