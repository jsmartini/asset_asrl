#include "ODEPhaseBase.h"

#include "LGLControlSplines.h"
#include "LGLIntegrals.h"
#include "MeshSpacingConstraints.h"
#include "PyDocString/OptimalControl/ODEPhaseBase_doc.h"
#include "ValueLock.h"

int ASSET::ODEPhaseBase::calc_threads()
{
    if (this->Threads > 1&&false) { 
        return this->Threads; 
    }

    auto Cost = [](int N,int T,int Tmax, int V) {
        int N_t = N / T;
        int Nmt = N_t % V;
        int NN = N % T;
        int NN_t = NN / T;
        int NNmt = NN_t % V;

        double tscale = double(T) / double(std::min(T, Tmax));


    };
    
    return 0;
}

int ASSET::ODEPhaseBase::addDeltaVarEqualCon(PhaseRegionFlags reg, int var,
                                             double value, double scale) {
  auto args = Arguments<2>();
  auto x0 = args.coeff<0>();
  auto x1 = args.coeff<1>();

  auto func = ((x1 - x0) - value) * scale;
  VectorXi v(1);
  v[0] = var;
  return this->addEqualCon(reg, func, v);
}

Eigen::VectorXi ASSET::ODEPhaseBase::addBoundaryValues(PhaseRegionFlags reg,
                                                       VectorXi args,
                                                       const VectorXd& value) {
  if (args.size() != value.size()) {
      fmt::print(fmt::fg(fmt::color::red),
          "Transcription Error!!!\n"
          "Size of boundary value arguments({0:}) and size of boundary value vector({1:}) do not match\n",
          args.size(), value.size());
    throw std::invalid_argument("");
  }
  auto Args = Arguments<1>();
  VectorXi fnums(args.size());
  for (int i = 0; i < args.size(); i++) {
    VectorXi atemp(1);
    atemp[0] = args[i];
    auto bv = Args - value[i];
    fnums[i] = this->addEqualCon(StateConstraint(bv, reg, atemp));
  }
  return fnums;
}

int ASSET::ODEPhaseBase::addValueLock(PhaseRegionFlags reg, VectorXi args) {
  return this->addEqualCon(
      StateConstraint(LockArgs<-1>(args.size()), reg, args));
  ;
}

int ASSET::ODEPhaseBase::addBoundaryValue(PhaseRegionFlags reg, VectorXi args,
                                          const VectorXd& value) {
  if (args.size() != value.size()) {
      fmt::print(fmt::fg(fmt::color::red),
          "Transcription Error!!!\n"
          "Size of boundary value arguments({0:}) and size of boundary value vector({1:}) do not match\n",
          args.size(), value.size());
      throw std::invalid_argument("");
  }
  auto Func = Arguments<-1>(args.size()) - value;
  return this->addEqualCon(StateConstraint(Func, reg, args));
}

int ASSET::ODEPhaseBase::addPeriodicityCon(VectorXi args)
{
    auto X = Arguments<-1>(args.size() * 2);
    auto Func = X.head(args.size()) - X.tail(args.size());
    return this->addEqualCon(StateConstraint(Func, PhaseRegionFlags::FrontandBack, args));
}

int ASSET::ODEPhaseBase::addLUVarBound(PhaseRegionFlags reg, int var,
                                       double lowerbound, double upperbound,
                                       double lbscale, double ubscale) {
  
    
  if (lowerbound > upperbound) {
      fmt::print(fmt::fg(fmt::color::red),
          "Transcription Error!!!\n"
          "Lower-bound({0:.3e}) greater than Upper-bound({1:.3e}) \n",
          lowerbound, upperbound);
      throw std::invalid_argument("");
  }
  check_lbscale(lbscale);
  check_ubscale(ubscale);

  auto x = Arguments<1>();
  auto lowbound = (lowerbound - x) * lbscale;
  auto ubound = (x - upperbound) * ubscale;

  auto lubound = StackedOutputs{lowbound, ubound};
  VectorXi v(1);
  v[0] = var;
  return this->addInequalCon(StateConstraint(lubound, reg, v));
}

int ASSET::ODEPhaseBase::addLowerVarBound(PhaseRegionFlags reg, int var,
                                          double lowerbound, double lbscale) {


  check_lbscale(lbscale);

  auto x = Arguments<1>();
  auto lbound = (lowerbound - x) * lbscale;
  VectorXi v(1);
  v[0] = var;

  return this->addInequalCon(StateConstraint(lbound, reg, v));
}

int ASSET::ODEPhaseBase::addUpperVarBound(PhaseRegionFlags reg, int var,
                                          double upperbound, double ubscale) {


  check_ubscale(ubscale);


  auto x = Arguments<1>();
  auto ubound = (x - upperbound) * ubscale;
  VectorXi v(1);
  v[0] = var;
  return this->addInequalCon(StateConstraint(ubound, reg, v));
}

int ASSET::ODEPhaseBase::addLUNormBound(PhaseRegionFlags reg, VectorXi vars,
                                        double lowerbound, double upperbound,
                                        double lbscale, double ubscale) {


    if (lowerbound > upperbound) {
        fmt::print(fmt::fg(fmt::color::red),
            "Transcription Error!!!\n"
            "Lower-bound({0:.3e}) greater than Upper-bound({1:.3e}) \n",
            lowerbound, upperbound);
        throw std::invalid_argument("");
    }
    check_lbscale(lbscale);
    check_ubscale(ubscale);

  int size = vars.size();
  auto impl = [&](auto sz) {
    auto x = Arguments<1>();
    auto lubound =
        StackedOutputs{(lowerbound - x) * lbscale, (x - upperbound) * ubscale};
    auto normfun = lubound.eval(Arguments<sz.value>(size).norm());
    return this->addInequalCon(StateConstraint(normfun, reg, vars));
  };

  switch (size) {
    
    case 2:
      return impl(int_const<2>());
    case 3:
      return impl(int_const<3>());
    case 4:
      return impl(int_const<4>());
    default:
      return impl(int_const<-1>());
  }
  return 0;
}

int ASSET::ODEPhaseBase::addLowerNormBound(PhaseRegionFlags reg, VectorXi vars,
                                           double lowerbound, double lbscale) {


  check_lbscale(lbscale);

  int size = vars.size();
  auto impl = [&](auto sz) {
    auto x = Arguments<1>();
    auto normfun = (lowerbound - Arguments<sz.value>(size).norm()) * lbscale;
    return this->addInequalCon(StateConstraint(normfun, reg, vars));
  };
  switch (size) {
    case 2:
      return impl(int_const<2>());
    case 3:
      return impl(int_const<3>());
    case 4:
      return impl(int_const<4>());
    default:
      return impl(int_const<-1>());
  }
}

int ASSET::ODEPhaseBase::addUpperNormBound(PhaseRegionFlags reg, VectorXi vars,
                                           double upperbound, double ubscale) {


  check_ubscale(ubscale);


  int size = vars.size();
  auto impl = [&](auto sz) {
    auto x = Arguments<1>();
    auto normfun = (Arguments<sz.value>(size).norm() - upperbound) * ubscale;
    return this->addInequalCon(StateConstraint(normfun, reg, vars));
  };
  switch (size) {
    case 2:
      return impl(int_const<2>());
    case 3:
      return impl(int_const<3>());
    case 4:
      return impl(int_const<4>());
    default:
      return impl(int_const<-1>());
  }
}



int ASSET::ODEPhaseBase::addLUSquaredNormBound(PhaseRegionFlags reg, VectorXi vars,
    double lowerbound, double upperbound,
    double lbscale, double ubscale) {


    if (lowerbound > upperbound) {
        fmt::print(fmt::fg(fmt::color::red),
            "Transcription Error!!!\n"
            "Lower-bound({0:.3e}) greater than Upper-bound({1:.3e}) \n",
            lowerbound, upperbound);
        throw std::invalid_argument("");
    }
    check_lbscale(lbscale);
    check_ubscale(ubscale);

    int size = vars.size();
    auto impl = [&](auto sz) {
        auto x = Arguments<1>();
        auto lubound =
            StackedOutputs{ (lowerbound - x) * lbscale, (x - upperbound) * ubscale };
        auto normfun = lubound.eval(Arguments<sz.value>(size).squared_norm());
        return this->addInequalCon(StateConstraint(normfun, reg, vars));
    };

    switch (size) {

    case 2:
        return impl(int_const<2>());
    case 3:
        return impl(int_const<3>());
    case 4:
        return impl(int_const<4>());
    default:
        return impl(int_const<-1>());
    }
    return 0;
}

int ASSET::ODEPhaseBase::addLowerSquaredNormBound(PhaseRegionFlags reg, VectorXi vars,
    double lowerbound, double lbscale) {


    check_lbscale(lbscale);

    int size = vars.size();
    auto impl = [&](auto sz) {
        auto x = Arguments<1>();
        auto normfun = (lowerbound - Arguments<sz.value>(size).squared_norm()) * lbscale;
        return this->addInequalCon(StateConstraint(normfun, reg, vars));
    };
    switch (size) {
    case 2:
        return impl(int_const<2>());
    case 3:
        return impl(int_const<3>());
    case 4:
        return impl(int_const<4>());
    default:
        return impl(int_const<-1>());
    }
    return 0;
}

int ASSET::ODEPhaseBase::addUpperSquaredNormBound(PhaseRegionFlags reg, VectorXi vars,
    double upperbound, double ubscale) {


    check_ubscale(ubscale);


    int size = vars.size();
    auto impl = [&](auto sz) {
        auto x = Arguments<1>();
        auto normfun = (Arguments<sz.value>(size).squared_norm() - upperbound) * ubscale;
        return this->addInequalCon(StateConstraint(normfun, reg, vars));
    };
    switch (size) {
    case 2:
        return impl(int_const<2>());
    case 3:
        return impl(int_const<3>());
    case 4:
        return impl(int_const<4>());
    default:
        return impl(int_const<-1>());
    }
    return 0;
}




int ASSET::ODEPhaseBase::addLowerFuncBound(PhaseRegionFlags reg,
                                           ScalarFunctionalX func,
                                           VectorXi vars, double lowerbound,
                                           double lbscale) {

  check_lbscale(lbscale);

  Vector1<double> rhs;
  rhs[0] = lowerbound;
  auto lbfun = ((-1.0 * func) + rhs) * lbscale;
  return this->addInequalCon(StateConstraint(lbfun, reg, vars));
}
int ASSET::ODEPhaseBase::addUpperFuncBound(PhaseRegionFlags reg,
                                           ScalarFunctionalX func,
                                           VectorXi vars, double upperbound,
                                           double ubscale) {


  check_ubscale(ubscale);


  Vector1<double> rhs;
  rhs[0] = -upperbound;
  auto lbfun = ((func) + rhs) * ubscale;
  
  return this->addInequalCon(StateConstraint(lbfun, reg, vars));
}
int ASSET::ODEPhaseBase::addLUFuncBound(PhaseRegionFlags reg, ScalarFunctionalX func, VectorXi vars, double lowerbound, double upperbound, double lbscale, double ubscale)
{
    if (lowerbound > upperbound) {
        fmt::print(fmt::fg(fmt::color::red),
            "Transcription Error!!!\n"
            "Lower-bound({0:.3e}) greater than Upper-bound({1:.3e}) \n",
            lowerbound, upperbound);
        throw std::invalid_argument("");
    }
    check_lbscale(lbscale);
    check_ubscale(ubscale);
    auto x = Arguments<1>();
    auto lubound =
        StackedOutputs{ (lowerbound - x) * lbscale, (x - upperbound) * ubscale };
    auto lufun = lubound.eval(func);
    return this->addInequalCon(StateConstraint(lufun, reg, vars));
}
////////////////////////////////////////////////////////////
int ASSET::ODEPhaseBase::addLowerDeltaVarBound(PhaseRegionFlags reg, int var,
                                               double lowerbound,
                                               double lbscale) {

  check_lbscale(lbscale);


  auto args = Arguments<2>();
  auto x0 = args.coeff<0>();
  auto x1 = args.coeff<1>();
  auto func = (lowerbound - (x1 - x0)) * lbscale;
  VectorXi v(1);
  v[0] = var;
  return this->addInequalCon(reg, func, v);
}

int ASSET::ODEPhaseBase::addUpperDeltaVarBound(PhaseRegionFlags reg, int var,
                                               double upperbound,
                                               double ubscale) {


  check_ubscale(ubscale);

  auto args = Arguments<2>();
  auto x0 = args.coeff<0>();
  auto x1 = args.coeff<1>();
  auto func = ((x1 - x0) - upperbound) * ubscale;
  VectorXi v(1);
  v[0] = var;
  return this->addInequalCon(reg, func, v);
}

int ASSET::ODEPhaseBase::addValueObjective(PhaseRegionFlags reg, int var,
                                           double scale) {
  auto obj = Arguments<1>() * scale;
  VectorXi v(1);
  v[0] = var;
  return this->addStateObjective(StateObjective(obj, reg, v));
}

int ASSET::ODEPhaseBase::addDeltaVarObjective(int var, double scale) {
  auto args = Arguments<2>();
  auto x0 = args.coeff<0>();
  auto x1 = args.coeff<1>();
  auto func = ((x1 - x0)) * scale;
  VectorXi v(1);
  v[0] = var;
  return this->addStateObjective(PhaseRegionFlags::FrontandBack, func, v);
}

std::vector<Eigen::VectorXd> ASSET::ODEPhaseBase::returnCostateTraj() const {
  if (!this->MultipliersLoaded) {
    throw std::invalid_argument(
        "No costates to return,a solve or optimize call must be made before "
        "returning the costate trajectory ");
  }
  auto TrajTemp = this->indexer.getFuncEqMultipliers(this->DynamicsFuncIndex,
                                                     this->ActiveEqLmults);

  
  std::vector<Eigen::VectorXd> tmp;
  int k = 0;
  int stride = (this->numTranCardStates - 1);
  auto getSpace = [&](int i) {
    if (this->numTranCardStates == 4) {
      return LGLCoeffs<4>::InteriorSpacings[i];
    } else if (this->numTranCardStates == 3) {
      return LGLCoeffs<3>::InteriorSpacings[i];
    } else if (this->numTranCardStates == 2) {
      return LGLCoeffs<2>::InteriorSpacings[i];
    } else {
      std::invalid_argument(
          "Costate estimation Not Implemented for specified Transcription "
          "Mode");
      return 0.0;
    }
  };
  for (auto& T : TrajTemp) {
    VectorXd x0 = this->ActiveTraj[k * (stride)];
    VectorXd xf = this->ActiveTraj[k * (stride) + stride];
    double t0 = x0[this->TVar()];
    double h = xf[this->TVar()] - x0[this->TVar()];
    for (int i = 0; i < stride; i++) {
      VectorXd cs(this->XVars() + 1);
      cs.head(this->XVars()) = T.segment(i * this->XVars(), this->XVars());
      cs[this->TVar()] = t0 + h * getSpace(i);
      tmp.push_back(cs);
    }
    k++;
  }

  std::vector<Eigen::VectorXd> Costates(this->ActiveTraj.size());
  
  for (int i = 0; i < (this->ActiveTraj.size()); i++) {

    int idx0 = i - 1;
    int idx1 = i;

    if (i == 0) {
        idx0++;
        idx1++;
    }
    else if (i == (this->ActiveTraj.size() - 1)) {
        idx0--;
        idx1--;
    }
    auto t0 = tmp[idx0][this->TVar()];
    auto t1 = tmp[idx1][this->TVar()];
    auto tm = this->ActiveTraj[i][this->TVar()];
    Costates[i] = tmp[idx0] + ((tm - t0) / (t1 - t0)) * (tmp[idx1] - tmp[idx0]);
  }

  
  return Costates;
}

void ASSET::ODEPhaseBase::setTraj(const std::vector<Eigen::VectorXd>& mesh,
                                  Eigen::VectorXd DBS, Eigen::VectorXi DPB) {

  if (mesh.size() == 0) {
        throw std::invalid_argument("Input trajectory is empty");
  }
  if (DPB.sum() < 2) {
      throw std::invalid_argument("Phase must have least 2 segments/defects.");
  }
  int msize = mesh[0].size();
  if (msize != this->Table.XtUVars) {
    std::cout << "User Input Error in function setInitTraj for ODE:"
              << this->Table.ode.name() << std::endl;
    std::cout << " Dimension of Input States(" << msize
              << ") does not match expected dimensions of the ODE("
              << this->Table.XtUVars << ")" << std::endl;
    throw std::invalid_argument("");
  }
  if ((DBS.size() - 1) != DPB.size()) {
    std::cout << "User Input Error in function setInitTraj for ODE:"
              << this->Table.ode.name() << std::endl;
    std::cout << "  Size of Defect Bin Spacing(" << DBS.size()
              << ") not consistent with size of Defects Per Bin(" << DPB.size()
              << ")" << std::endl;
    throw std::invalid_argument("");
  }
  for (auto& X : mesh) {
      if (X.hasNaN()) {
          throw std::invalid_argument("NaN detetected in State Vector in ODEPhaseBase::setTraj");
      }
  }

  this->Table.loadUnevenData(DPB.sum(), mesh);
  this->ActiveTraj = this->Table.NDdistribute(DBS, DPB);
  this->DefBinSpacing = DBS;
  this->DefsPerBin = DPB;
  this->numDefects = DPB.sum();
  this->TrajectoryLoaded = true;
  this->resetTranscription();
}

void ASSET::ODEPhaseBase::refineTrajManual(VectorXd DBS, VectorXi DPB) {
  if ((DBS.size() - 1) != DPB.size()) {
    std::cout << "User Input Error in function setInitTraj for ODE:"
              << this->Table.ode.name() << std::endl;
    std::cout << "  Size of Defect Bin Spacing(" << DBS.size()
              << ") not consistent with size of Defects Per Bin(" << DPB.size()
              << ")" << std::endl;
    throw std::invalid_argument("");
  }

  this->Table.loadRegularData(DPB.sum(), this->ActiveTraj);
  this->ActiveTraj = this->Table.NDdistribute(DBS, DPB);
  this->DefBinSpacing = DBS;
  this->DefsPerBin = DPB;
  this->numDefects = DPB.sum();
  this->resetTranscription();
}

std::vector<Eigen::VectorXd> ASSET::ODEPhaseBase::refineTrajEqual(int n) {
  LGLInterpTable tcopy = this->Table;
  tcopy.loadExactData(this->ActiveTraj);
  std::vector<Eigen::VectorXd> errint = tcopy.ErrorIntegral(n * 8);
  std::vector<Eigen::VectorXd> erintdt = FDiffData(errint, 1, false);
  LGLInterpTable etab(1, 0, LGL3);
  etab.loadEvenData2(errint, erintdt);

  Eigen::VectorXd etargs;
  etargs.setLinSpaced(n + 1, errint[0][0], errint.back()[0]);
  Eigen::VectorXd bins;
  bins.setLinSpaced(n + 1, 0.0, 1.0);
  Eigen::VectorXd ts;
  ts.setLinSpaced(n + 1, etab.T0, etab.TF);
  double low = etab.T0;

  Eigen::VectorXd evl;
  Eigen::VectorXd evh;
  Eigen::VectorXd evm;
  for (int i = 1; i < n; i++) {
    double high = etab.TF;
    double mid = (high + low) / 2.0;
    evl = etab.Interpolate(low);
    evh = etab.Interpolate(high);
    for (int j = 0; j < 35; j++) {
      evm = etab.Interpolate(mid);
      if (evm[0] < etargs[i]) {
        low = mid;
        evl = etab.Interpolate(low);
      } else {
        high = mid;
        evh = etab.Interpolate(high);
      }
      mid = (high + low) / 2.0;
    }
    bins[i] = (mid - etab.T0) / etab.TotalT;
  }

  Eigen::VectorXi dpb = VectorXi::Ones(n);

  this->refineTrajManual(bins, dpb);

  return tcopy.NDdistribute(bins, dpb);
}

void ASSET::ODEPhaseBase::subVariables(PhaseRegionFlags reg, VectorXi indices,
                                       VectorXd vals) {
  switch (reg) {
    case PhaseRegionFlags::Front: {
      for (int i = 0; i < indices.size(); i++) {
        this->ActiveTraj[0][indices[i]] = vals[i];
      }
      break;
    }
    case PhaseRegionFlags::Back: {
      for (int i = 0; i < indices.size(); i++) {
        this->ActiveTraj.back()[indices[i]] = vals[i];
      }
      break;
    }
    case PhaseRegionFlags::Path: {
      for (int i = 0; i < indices.size(); i++) {
        for (int j = 0; j < this->ActiveTraj.size(); j++) {
          this->ActiveTraj[j][indices[i]] = vals[i];
        }
      }
      break;
    }
    case PhaseRegionFlags::StaticParams: {
      for (int i = 0; i < indices.size(); i++) {
        this->ActiveStaticParams[indices[i]] = vals[i];
      }
      break;
    }
    default: {
      throw std::invalid_argument(
          "Variable Substitution Not Allowed for specified Phase Region");
    }
  }
}

void ASSET::ODEPhaseBase::transcribe_integrals() {
  auto MakeInt = [&](auto cs, auto xv, auto pv,
                     const ScalarFunctionalX& integrand, int xvv, int pvv) {
    auto integral =
        LGLIntegral<ScalarFunctionalX, cs.value, xv.value, pv.value>{integrand,
                                                                     xvv, pvv};
    integral.EnableVectorization = this->EnableVectorization;
    
    return ObjectiveInterface(integral);
  };
  auto SwitchX = [&](auto cs, int xv, auto pv,
                     const ScalarFunctionalX& integrand, int xvv, int pvv) {
    switch (xv) {
      case 1:
        return MakeInt(cs, int_const<1>(), pv, integrand, xvv, pvv);
      case 2:
        return MakeInt(cs, int_const<2>(), pv, integrand, xvv, pvv);
      case 3:
        return MakeInt(cs, int_const<3>(), pv, integrand, xvv, pvv);
      // case 4:
      //   return MakeInt(cs, int_const<4>(), pv, integrand, xvv, pvv);
      default:
        return MakeInt(cs, int_const<-1>(), pv, integrand, xvv, pvv);
    }
  };
  auto SwitchP = [&](auto cs, int xv, int pv,
                     const ScalarFunctionalX& integrand, int xvv, int pvv) {
    switch (pv) {
      case 0:
        return SwitchX(cs, xv, int_const<0>(), integrand, xvv, pvv);
      // case 1:
      //   return SwitchX(cs, xv, int_const<1>(), integrand, xvv, pvv);
      default:
        return MakeInt(cs, int_const<-1>(), int_const<-1>(), integrand, xvv,
                       pvv);
    }
  };
  auto SwitchC = [&](int cs, int xv, int pv, const ScalarFunctionalX& integrand,
                     int xvv, int pvv) {
    switch (cs) {
      case 2:
        return SwitchP(int_const<2>(), xv, pv, integrand, xvv, pvv);
      case 3:
        return SwitchP(int_const<3>(), xv, pv, integrand, xvv, pvv);
      case 4:
        return SwitchP(int_const<4>(), xv, pv, integrand, xvv, pvv);
    }
  };

  for (auto& ob : this->userIntegrands) {
    int xp = ob.XtUVars.size();
    int sop = ob.OPVars.size() + ob.SPVars.size();
    VectorXi xtrap(xp + 1);
    xtrap.head(xp) = ob.XtUVars;
    xtrap[xp] = this->TVar();

    ObjectiveInterface obj;
    PhaseRegionFlags PhaseReg = PhaseRegionFlags::PairWisePath;

    if (this->IntegralMode == IntegralModes::BaseIntegral &&
        ((this->TranscriptionMode == LGL5) ||
         (this->TranscriptionMode == LGL7))) {
      if (this->TranscriptionMode == LGL5) {
        obj = SwitchC(3, xp, sop, ob.Func, xp, sop);
      } else if (this->TranscriptionMode == LGL7) {
        obj = SwitchC(4, xp, sop, ob.Func, xp, sop);
      }
      PhaseReg = PhaseRegionFlags::DefectPath;
    } else {
      obj = SwitchC(2, xp, sop, ob.Func, xp, sop);
      if (this->TranscriptionMode == LGL3
          || this->TranscriptionMode == Trapezoidal
          || this->TranscriptionMode == CentralShooting) {
          PhaseReg = PhaseRegionFlags::DefectPath;

      }
      else {
          PhaseReg = PhaseRegionFlags::PairWisePath;

      }
    }

    ThreadingFlags ThreadMode = ob.Func.thread_safe()
                                    ? ThreadingFlags::ByApplication
                                    : ThreadingFlags::MainThread;

    int Gindex = this->indexer.addObjective(obj, PhaseReg, xtrap, ob.OPVars,
                                            ob.SPVars, ThreadMode);

    int PLindex = Gindex - this->indexer.StartObj;
    ob.GlobalIndex = Gindex;
    ob.PhaseLocalIndex = PLindex;
  }

  for (auto& ob : this->userParamIntegrands) {
    int xp = ob.XtUVars.size();
    int sop = ob.OPVars.size() + ob.SPVars.size();
    VectorXi xtrap(xp + 1);
    xtrap.head(xp) = ob.XtUVars;
    xtrap[xp] = this->TVar();

    ObjectiveInterface obj;
    PhaseRegionFlags PhaseReg = PhaseRegionFlags::PairWisePath;

    if (this->IntegralMode == IntegralModes::BaseIntegral &&
        ((this->TranscriptionMode == LGL5) ||
         (this->TranscriptionMode == LGL7))) {
      if (this->TranscriptionMode == LGL5) {
        obj = SwitchC(3, xp, sop, ob.Func, xp, sop);
      } else if (this->TranscriptionMode == LGL7) {
        obj = SwitchC(4, xp, sop, ob.Func, xp, sop);
      }
      PhaseReg = PhaseRegionFlags::DefectPath;
    } else {
      obj = SwitchC(2, xp, sop, ob.Func, xp, sop);
      PhaseReg = PhaseRegionFlags::PairWisePath;
    }

    ThreadingFlags ThreadMode = ob.Func.thread_safe()
                                    ? ThreadingFlags::ByApplication
                                    : ThreadingFlags::MainThread;

    auto AccFunc = Arguments<1>() * -1.0;

    int Gindex = this->indexer.addAccumulation(obj, PhaseReg, xtrap, ob.OPVars,
                                               ob.SPVars, AccFunc, ob.EXTVars,
                                               ThreadMode);

    int PLindex = Gindex - this->indexer.StartObj;
    ob.GlobalIndex = Gindex;
    ob.PhaseLocalIndex = PLindex;
  }
}
void ASSET::ODEPhaseBase::transcribe_basic_funcs() {
  for (auto& eq : this->userEqualities) {
    ThreadingFlags ThreadMode = eq.Func.thread_safe()
                                    ? ThreadingFlags::ByApplication
                                    : ThreadingFlags::MainThread;
    if (eq.RegionFlag == PhaseRegionFlags::Path || eq.RegionFlag == PhaseRegionFlags::PairWisePath)
        eq.Func.enable_vectorization(this->EnableVectorization);
    int Gindex = this->indexer.addEquality(eq.Func, eq.RegionFlag, eq.XtUVars,
                                           eq.OPVars, eq.SPVars, ThreadMode);
   
    int PLindex = Gindex - this->indexer.StartEq;
    eq.GlobalIndex = Gindex;
    eq.PhaseLocalIndex = PLindex;
  }
  for (auto& iq : this->userInequalities) {
    ThreadingFlags ThreadMode = iq.Func.thread_safe()
                                    ? ThreadingFlags::ByApplication
                                    : ThreadingFlags::MainThread;
    if (iq.RegionFlag == PhaseRegionFlags::Path || iq.RegionFlag == PhaseRegionFlags::PairWisePath) {
        
        iq.Func.enable_vectorization(this->EnableVectorization);

    }

    int Gindex = this->indexer.addInequality(iq.Func, iq.RegionFlag, iq.XtUVars,
                                             iq.OPVars, iq.SPVars, ThreadMode);
    int PLindex = Gindex - this->indexer.StartIq;
    iq.GlobalIndex = Gindex;
    iq.PhaseLocalIndex = PLindex;
  }
  for (auto& ob : this->userStateObjectives) {
    ThreadingFlags ThreadMode = ob.Func.thread_safe()
                                    ? ThreadingFlags::ByApplication
                                    : ThreadingFlags::MainThread;
    if (ob.RegionFlag == PhaseRegionFlags::Path || ob.RegionFlag == PhaseRegionFlags::PairWisePath)
        ob.Func.enable_vectorization(this->EnableVectorization);
    int Gindex = this->indexer.addObjective(ob.Func, ob.RegionFlag, ob.XtUVars,
                                            ob.OPVars, ob.SPVars, ThreadMode);
    int PLindex = Gindex - this->indexer.StartObj;
    ob.GlobalIndex = Gindex;
    ob.PhaseLocalIndex = PLindex;
  }
}
void ASSET::ODEPhaseBase::transcribe_axis_funcs() {
  VectorXd cspace(this->numDefects + 1);
  int start = 0;
  for (int i = 0; i < this->DefsPerBin.size(); i++) {
    cspace.segment(start, this->DefsPerBin[i] + 1)
        .setLinSpaced(this->DefBinSpacing[i], this->DefBinSpacing[i + 1]);
    start += this->DefsPerBin[i];
  }

  std::vector<ConstraintInterface> AxisFuncs;
  std::vector<int> Tmodes;
  Eigen::VectorXi bins(this->Threads + 1);
  bins.setLinSpaced(0, this->indexer.numNodalStates);

  for (int i = 0; i < this->indexer.numNodalStates - 2; i++) {
    AxisFuncs.emplace_back(SingleMeshSpacing(cspace[i + 1]));
    int thrt = Thread0;
    for (int j = 0; j < this->Threads; j++) {
      if (i >= bins(j) && i < bins(j + 1)) {
        thrt =j;
      }
    }
    Tmodes.push_back(thrt);
  }

  VectorXi tloc(1);
  tloc[0] = this->TVar();

  VectorXi empty(0);
  empty.resize(0);

  if (this->TranscriptionMode == TranscriptionModes::LGL7) {
    LGLMeshSpacing<4> axcon7;
    this->indexer.addEquality(axcon7, PhaseRegionFlags::DefectPath, tloc, empty,
                              empty, ThreadingFlags::ByApplication);
  } else if (this->TranscriptionMode == TranscriptionModes::LGL5) {
    LGLMeshSpacing<3> axcon5;
    this->indexer.addEquality(axcon5, PhaseRegionFlags::DefectPath, tloc, empty,
                              empty, ThreadingFlags::ByApplication);
  }

  this->indexer.addPartitionedEquality(AxisFuncs,
                                       PhaseRegionFlags::FrontNodalBackPath,
                                       tloc, empty, empty, Tmodes);
}

void ASSET::ODEPhaseBase::transcribe_control_funcs() {
  VectorXi StateT(this->XtUVars());
  for (int i = 0; i < this->XtUVars(); i++) StateT[i] = i;
  VectorXi TUvarT = StateT.segment(this->TVar(), 1 + this->UVars());
  VectorXi empty(0);
  empty.resize(0);

  std::vector<VectorXi> TUis(this->UVars());
  for (int i = 0; i < this->UVars(); i++) {
    TUis[i].resize(2);
    TUis[i][0] = this->TVar();
    TUis[i][1] = this->TVar() + 1 + i;
  }

  if (this->TranscriptionMode == TranscriptionModes::LGL7) {
    if (this->UVars() > 0) {
      if (this->ControlMode == ControlModes::HighestOrderSpline) {
        LGLControlSpline<4, -1, 2> lgl7spln2(this->UVars());
        
        this->indexer.addEquality(lgl7spln2,
                                  PhaseRegionFlags::DefectPairWisePath, TUvarT,
                                  empty, empty, ThreadingFlags::ByApplication);

      } else if (this->ControlMode == ControlModes::FirstOrderSpline) {
        LGLControlSpline<4, -1, 1> lgl7spln1(this->UVars());
        
        this->indexer.addEquality(lgl7spln1,
                                  PhaseRegionFlags::DefectPairWisePath, TUvarT,
                                  empty, empty, ThreadingFlags::ByApplication);
      }
    }
  } else if (this->TranscriptionMode == TranscriptionModes::LGL5) {
    if (this->UVars() > 0) {
      if (this->ControlMode == ControlModes::HighestOrderSpline ||
          this->ControlMode == ControlModes::FirstOrderSpline) {
        LGLControlSpline<3, -1, 1> lgl5spln1(this->UVars());
        
        this->indexer.addEquality(lgl5spln1,
                                  PhaseRegionFlags::DefectPairWisePath, TUvarT,
                                  empty, empty, ThreadingFlags::ByApplication);
      }
    }
  }
}

void ASSET::ODEPhaseBase::check_functions(int pnum) {
  auto CheckFun = [&](const std::string& type, auto& func) {
    if (func.XtUVars.size() > 0) {
      if (func.XtUVars.maxCoeff() >= this->XtUPVars() ||
          func.XtUVars.minCoeff() < 0) {

          fmt::print(fmt::fg(fmt::color::red),
              "Transcription Error!!!\n"
              "{0:} function state variable indices out of bounds in phase:{1:}\n"
              " Function Storage Index:{2:}\n"
              " Function Name:{3:}\n",
              type,pnum, func.StorageIndex, func.Func.name());
          throw std::invalid_argument("");

      }
    }
    if (func.OPVars.size() > 0) {
      if (func.OPVars.maxCoeff() >= this->PVars() ||
          func.OPVars.minCoeff() < 0) {

          fmt::print(fmt::fg(fmt::color::red),
              "Transcription Error!!!\n"
              "{0:} function ODE Param variable indices out of bounds in phase:{1:}\n"
              " Function Storage Index:{2:}\n"
              " Function Name:{3:}\n",
              type, pnum, func.StorageIndex, func.Func.name());
          throw std::invalid_argument("");

      }
    }
    if (func.SPVars.size() > 0) {
      if (func.SPVars.maxCoeff() >= this->numStatParams ||
          func.SPVars.minCoeff() < 0) {
          fmt::print(fmt::fg(fmt::color::red),
              "Transcription Error!!!\n"
              "{0:} function Static Param variable indices out of bounds in phase:{1:}\n"
              " Function Storage Index:{2:}\n"
              " Function Name:{3:}\n",
              type, pnum, func.StorageIndex, func.Func.name());
          throw std::invalid_argument("");
      }
    }
    if (func.EXTVars.size() > 0) {
      if (func.EXTVars.maxCoeff() >= this->numStatParams ||
          func.EXTVars.minCoeff() < 0) {

          fmt::print(fmt::fg(fmt::color::red),
              "Transcription Error!!!\n"
              "{0:} function Integral Static Param variable indices out of bounds in phase:{1:}\n"
              " Function Storage Index:{2:}\n"
              " Function Name:{3:}\n",
              type, pnum, func.StorageIndex, func.Func.name());
          throw std::invalid_argument("");
      }
    }
  };

  std::string eq = "Equality constraint";
  std::string iq = "Inequality constraint";
  std::string sobj = "State objective";
  std::string iobj = "Integral objective";
  std::string ipcon = "Integral parameter";

  for (auto& f : this->userEqualities) CheckFun(eq, f);
  for (auto& f : this->userInequalities) CheckFun(iq, f);
  for (auto& f : this->userIntegrands) CheckFun(iobj, f);
  for (auto& f : this->userParamIntegrands) CheckFun(ipcon, f);
  for (auto& f : this->userStateObjectives) CheckFun(sobj, f);
}

void ASSET::ODEPhaseBase::transcribe_phase(int vo, int eqo, int iqo,
                                           std::shared_ptr<NonLinearProgram> np,
                                           int pnum)

{
  this->indexer.begin_indexing(np, vo, eqo, iqo);
  this->check_functions(pnum);

  
  this->transcribe_dynamics();
  this->transcribe_axis_funcs();
  this->transcribe_control_funcs();
  this->transcribe_integrals();
  this->transcribe_basic_funcs();
  

  

  //////DO NOT GET RID OF THIS!!!!!!//
  this->doTranscription = false;
  ////////////////////////////////////
}

void ASSET::ODEPhaseBase::transcribe(bool showstats, bool showfuns) {
  this->nlp = std::make_shared<NonLinearProgram>(this->Threads);

  this->initIndexing();
  this->transcribe_phase(0, 0, 0, this->nlp, 0);
  if (showstats) this->indexer.print_stats(showfuns);

  if (this->indexer.numPhaseEqCons > this->indexer.numPhaseVars) {
      fmt::print(fmt::fg(fmt::color::yellow),
          "Transcription Warning!!!\n"
          "Number of Equality Constraints({0:}) in phase exceeds number of free variables({1:}).\n"
          "You likely have a redundant constraint.\n",
          this->indexer.numPhaseEqCons, this->indexer.numPhaseVars);

  }

  this->nlp->make_NLP(this->indexer.numPhaseVars, this->indexer.numPhaseEqCons,
                      this->indexer.numPhaseIqCons);

  this->optimizer->setNLP(this->nlp);
}

void ASSET::ODEPhaseBase::test_threads(int i, int j, int n) {
  this->resetTranscription();

  auto nlp1 = std::make_shared<NonLinearProgram>(i);
  this->initIndexing();
  this->transcribe_phase(0, 0, 0, nlp1, 0);
  if (false) this->indexer.print_stats(false);
  nlp1->make_NLP(this->indexer.numPhaseVars, this->indexer.numPhaseEqCons,
                 this->indexer.numPhaseIqCons);

  auto nlp2 = std::make_shared<NonLinearProgram>(j);
  this->initIndexing();
  this->transcribe_phase(0, 0, 0, nlp2, 0);
  if (false) this->indexer.print_stats(false);
  nlp2->make_NLP(this->indexer.numPhaseVars, this->indexer.numPhaseEqCons,
                 this->indexer.numPhaseIqCons);

  Eigen::VectorXd v = this->makeSolverInput();
  NonLinearProgram::NLPTest(v, n, nlp1, nlp2);

  this->resetTranscription();
}

void ASSET::ODEPhaseBase::Build(py::module& m) {
  using namespace pybind11::literals;
  using namespace doc;
  auto obj = py::class_<ODEPhaseBase, std::shared_ptr<ODEPhaseBase>, OptimizationProblemBase>(
      m, "PhaseInterface");
  obj.doc() = "Base Class for All Optimal Control Phases";

  obj.def("enable_vectorization", &ODEPhaseBase::enable_vectorization);

  obj.def(
      "setTraj",
      py::overload_cast<const std::vector<Eigen::VectorXd>&, Eigen::VectorXd,
                        Eigen::VectorXi>(&ODEPhaseBase::setTraj),
      ODEPhaseBase_setTraj1);
  obj.def("setTraj",
          py::overload_cast<const std::vector<Eigen::VectorXd>&, int>(
              &ODEPhaseBase::setTraj),
          ODEPhaseBase_setTraj2);

  obj.def("switchTranscriptionMode",
          py::overload_cast<TranscriptionModes, VectorXd, VectorXi>(
              &ODEPhaseBase::switchTranscriptionMode),
          ODEPhaseBase_switchTranscriptionMode1);
  obj.def("switchTranscriptionMode",
          py::overload_cast<TranscriptionModes>(
              &ODEPhaseBase::switchTranscriptionMode),
          ODEPhaseBase_switchTranscriptionMode2);


  obj.def("switchTranscriptionMode",
      py::overload_cast<std::string, VectorXd, VectorXi>(
          &ODEPhaseBase::switchTranscriptionMode),
      ODEPhaseBase_switchTranscriptionMode1);
  obj.def("switchTranscriptionMode",
      py::overload_cast<std::string>(
          &ODEPhaseBase::switchTranscriptionMode),
      ODEPhaseBase_switchTranscriptionMode2);


  obj.def("transcribe",
          py::overload_cast<bool, bool>(&ODEPhaseBase::transcribe),
          ODEPhaseBase_transcribe);

  obj.def("refineTrajManual",
          py::overload_cast<int>(&ODEPhaseBase::refineTrajManual),
          ODEPhaseBase_refineTrajManual1);
  obj.def(
      "refineTrajManual",
      py::overload_cast<VectorXd, VectorXi>(&ODEPhaseBase::refineTrajManual),
      ODEPhaseBase_refineTrajManual2);
  obj.def("refineTrajEqual", &ODEPhaseBase::refineTrajEqual,
          ODEPhaseBase_refineTrajEqual);

  obj.def("setStaticParams", &ODEPhaseBase::setStaticParams,
          ODEPhaseBase_setStaticParams);


  obj.def("setControlMode", py::overload_cast<ControlModes>( & ODEPhaseBase::setControlMode),
          ODEPhaseBase_setControlMode);
  obj.def("setControlMode", py::overload_cast<std::string>(&ODEPhaseBase::setControlMode),
      ODEPhaseBase_setControlMode);

  obj.def("setIntegralMode", &ODEPhaseBase::setIntegralMode,
          ODEPhaseBase_setIntegralMode);

  obj.def("subStaticParams", &ODEPhaseBase::subStaticParams,
          ODEPhaseBase_subStaticParams);

  obj.def("subVariables", py::overload_cast<PhaseRegionFlags , VectorXi , VectorXd >(& ODEPhaseBase::subVariables),
          ODEPhaseBase_subVariables);
  obj.def("subVariable", py::overload_cast<PhaseRegionFlags, int, double>(&ODEPhaseBase::subVariable), 
      ODEPhaseBase_subVariable);

  obj.def("subVariables", py::overload_cast<std::string, VectorXi, VectorXd>(&ODEPhaseBase::subVariables),
      ODEPhaseBase_subVariables);
  obj.def("subVariable", py::overload_cast<std::string, int, double>(&ODEPhaseBase::subVariable),
      ODEPhaseBase_subVariable);

  obj.def("returnTraj", &ODEPhaseBase::returnTraj, ODEPhaseBase_returnTraj);
  obj.def("returnTrajRange", &ODEPhaseBase::returnTrajRange,
          ODEPhaseBase_returnTrajRange);
  obj.def("returnTrajRangeND", &ODEPhaseBase::returnTrajRangeND,
          ODEPhaseBase_returnTrajRangeND);
  obj.def("returnTrajTable", &ODEPhaseBase::returnTrajTable);

  obj.def("returnCostateTraj", &ODEPhaseBase::returnCostateTraj,
          ODEPhaseBase_returnCostateTraj);

  obj.def("returnEqualConLmults", &ODEPhaseBase::returnEqualConLmults,
          ODEPhaseBase_returnEqualConLmults);
  obj.def("returnInequalConLmults", &ODEPhaseBase::returnInequalConLmults,
          ODEPhaseBase_returnInequalConLmults);

  obj.def("returnStaticParams", &ODEPhaseBase::returnStaticParams,
          ODEPhaseBase_returnStaticParam);

  obj.def("test_threads", &ODEPhaseBase::test_threads);

  obj.def("removeEqualCon", &ODEPhaseBase::removeEqualCon,
          ODEPhaseBase_removeEqualCon);
  obj.def("removeInequalCon", &ODEPhaseBase::removeInequalCon,
          ODEPhaseBase_removeInequalCon);
  obj.def("removeStateObjective", &ODEPhaseBase::removeStateObjective,
          ODEPhaseBase_removeStateObjective);
  obj.def("removeIntegralObjective", &ODEPhaseBase::removeIntegralObjective,
          ODEPhaseBase_removeIntegralObjective);
  obj.def("removeIntegralParamFunction",
          &ODEPhaseBase::removeIntegralParamFunction,
          ODEPhaseBase_removeIntegralParamFunction);
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  obj.def("addEqualCon",
          py::overload_cast<StateConstraint>(&ODEPhaseBase::addEqualCon),
          ODEPhaseBase_addEqualCon1);
  obj.def("addEqualCon",
          py::overload_cast<PhaseRegionFlags, VectorFunctionalX, VectorXi>(
              &ODEPhaseBase::addEqualCon),
          ODEPhaseBase_addEqualCon2);
  obj.def("addEqualCon",
          py::overload_cast<PhaseRegionFlags, VectorFunctionalX, VectorXi,
                            VectorXi, VectorXi>(&ODEPhaseBase::addEqualCon),
          ODEPhaseBase_addEqualCon3);

  obj.def("addEqualCon",
      py::overload_cast<std::string, VectorFunctionalX, VectorXi>(
          &ODEPhaseBase::addEqualCon),
      ODEPhaseBase_addEqualCon2);
  obj.def("addEqualCon",
      py::overload_cast<std::string, VectorFunctionalX, VectorXi,
      VectorXi, VectorXi>(&ODEPhaseBase::addEqualCon),
      ODEPhaseBase_addEqualCon3);


  obj.def("addDeltaVarEqualCon",
          py::overload_cast<PhaseRegionFlags, int, double, double>(
              &ODEPhaseBase::addDeltaVarEqualCon),
          ODEPhaseBase_addDeltaVarEqualCon1);
  obj.def("addDeltaVarEqualCon",
          py::overload_cast<int, double, double>(
              &ODEPhaseBase::addDeltaVarEqualCon),
          ODEPhaseBase_addDeltaVarEqualCon2);
  obj.def("addDeltaVarEqualCon",
          py::overload_cast<int, double>(&ODEPhaseBase::addDeltaVarEqualCon),
          ODEPhaseBase_addDeltaVarEqualCon3);

  obj.def(
      "addDeltaTimeEqualCon",
      py::overload_cast<double, double>(&ODEPhaseBase::addDeltaTimeEqualCon),
      ODEPhaseBase_addDeltaTimeEqualCon1);
  obj.def("addDeltaTimeEqualCon",
          py::overload_cast<double>(&ODEPhaseBase::addDeltaTimeEqualCon),
          ODEPhaseBase_addDeltaTimeEqualCon2);

  obj.def("addBoundaryValue", py::overload_cast<PhaseRegionFlags , VectorXi ,const VectorXd &>
      (&ODEPhaseBase::addBoundaryValue),ODEPhaseBase_addBoundaryValue);
  obj.def("addBoundaryValue", py::overload_cast<std::string, VectorXi, const VectorXd&>
      (&ODEPhaseBase::addBoundaryValue), ODEPhaseBase_addBoundaryValue);

  obj.def("addValueLock", py::overload_cast<std::string, VectorXi>(&ODEPhaseBase::addValueLock),
          ODEPhaseBase_addValueLock);
  obj.def("addValueLock", py::overload_cast<PhaseRegionFlags, VectorXi>(&ODEPhaseBase::addValueLock),
      ODEPhaseBase_addValueLock);

  obj.def("addBoundaryValues", &ODEPhaseBase::addBoundaryValues,
          ODEPhaseBase_addBoundaryValues);

  obj.def("addPeriodicityCon", &ODEPhaseBase::addPeriodicityCon);
  ///////////////////////////////////////////////////////////////////////////////

  obj.def("addInequalCon",
          py::overload_cast<StateConstraint>(&ODEPhaseBase::addInequalCon),
          ODEPhaseBase_addInequalCon1);
  obj.def("addInequalCon",
          py::overload_cast<PhaseRegionFlags, VectorFunctionalX, VectorXi>(
              &ODEPhaseBase::addInequalCon),
          ODEPhaseBase_addInequalCon2);
  obj.def("addInequalCon",
          py::overload_cast<PhaseRegionFlags, VectorFunctionalX, VectorXi,
                            VectorXi, VectorXi>(&ODEPhaseBase::addInequalCon),
          ODEPhaseBase_addInequalCon3);

  obj.def("addInequalCon",
      py::overload_cast<std::string, VectorFunctionalX, VectorXi>(
          &ODEPhaseBase::addInequalCon),
      ODEPhaseBase_addInequalCon2);
  obj.def("addInequalCon",
      py::overload_cast<std::string, VectorFunctionalX, VectorXi,
      VectorXi, VectorXi>(&ODEPhaseBase::addInequalCon),
      ODEPhaseBase_addInequalCon3);


  obj.def("addLUVarBound",
          py::overload_cast<PhaseRegionFlags, int, double, double>(
              &ODEPhaseBase::addLUVarBound),
          ODEPhaseBase_addLUVarBound1);
  obj.def("addLUVarBound",
          py::overload_cast<PhaseRegionFlags, int, double, double, double>(
              &ODEPhaseBase::addLUVarBound),
          ODEPhaseBase_addLUVarBound2);
  obj.def(
      "addLUVarBound",
      py::overload_cast<PhaseRegionFlags, int, double, double, double, double>(
          &ODEPhaseBase::addLUVarBound),
      ODEPhaseBase_addLUVarBound3);
  obj.def("addLUVarBounds",
          py::overload_cast<PhaseRegionFlags, Eigen::VectorXi, double, double,
                            double>(&ODEPhaseBase::addLUVarBounds),
          ODEPhaseBase_addLUVarBounds);


  obj.def("addLUVarBound",
      py::overload_cast<std::string, int, double, double>(
          &ODEPhaseBase::addLUVarBound),
      ODEPhaseBase_addLUVarBound1);
  obj.def("addLUVarBound",
      py::overload_cast<std::string, int, double, double, double>(
          &ODEPhaseBase::addLUVarBound),
      ODEPhaseBase_addLUVarBound2);
  obj.def(
      "addLUVarBound",
      py::overload_cast<std::string, int, double, double, double, double>(
          &ODEPhaseBase::addLUVarBound),
      ODEPhaseBase_addLUVarBound3);
  obj.def("addLUVarBounds",
      py::overload_cast<std::string, Eigen::VectorXi, double, double,
      double>(&ODEPhaseBase::addLUVarBounds),
      ODEPhaseBase_addLUVarBounds);



  obj.def("addLowerVarBound",
          py::overload_cast<PhaseRegionFlags, int, double, double>(
              &ODEPhaseBase::addLowerVarBound),
          ODEPhaseBase_addLowerVarBound1);
  obj.def("addLowerVarBound",
          py::overload_cast<PhaseRegionFlags, int, double>(
              &ODEPhaseBase::addLowerVarBound),
          ODEPhaseBase_addLowerVarBound2);

  obj.def("addUpperVarBound",
          py::overload_cast<PhaseRegionFlags, int, double, double>(
              &ODEPhaseBase::addUpperVarBound),
          ODEPhaseBase_addUpperVarBound1);
  obj.def("addUpperVarBound",
          py::overload_cast<PhaseRegionFlags, int, double>(
              &ODEPhaseBase::addUpperVarBound),
          ODEPhaseBase_addUpperVarBound2);

  obj.def("addLowerVarBound",
      py::overload_cast<std::string, int, double, double>(
          &ODEPhaseBase::addLowerVarBound),
      ODEPhaseBase_addLowerVarBound1);
  obj.def("addLowerVarBound",
      py::overload_cast<std::string, int, double>(
          &ODEPhaseBase::addLowerVarBound),
      ODEPhaseBase_addLowerVarBound2);

  obj.def("addUpperVarBound",
      py::overload_cast<std::string, int, double, double>(
          &ODEPhaseBase::addUpperVarBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addUpperVarBound",
      py::overload_cast<std::string, int, double>(
          &ODEPhaseBase::addUpperVarBound),
      ODEPhaseBase_addUpperVarBound2);


  //////////////////////////////////////////////////////////////
  
  obj.def("addLowerNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double>(
          &ODEPhaseBase::addLowerNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addLowerNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double>(
          &ODEPhaseBase::addLowerNormBound),
      ODEPhaseBase_addUpperVarBound2);

  obj.def("addLowerNormBound",
      py::overload_cast<std::string, VectorXi, double, double>(
          &ODEPhaseBase::addLowerNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addLowerNormBound",
      py::overload_cast<std::string, VectorXi, double>(
          &ODEPhaseBase::addLowerNormBound),
      ODEPhaseBase_addUpperVarBound2);


  obj.def("addUpperNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double>(
          &ODEPhaseBase::addUpperNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addUpperNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double>(
          &ODEPhaseBase::addUpperNormBound),
      ODEPhaseBase_addUpperVarBound2);

  obj.def("addUpperNormBound",
      py::overload_cast<std::string, VectorXi, double, double>(
          &ODEPhaseBase::addUpperNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addUpperNormBound",
      py::overload_cast<std::string, VectorXi, double>(
          &ODEPhaseBase::addUpperNormBound),
      ODEPhaseBase_addUpperVarBound2);


  obj.def("addLUNormBound",
          py::overload_cast<PhaseRegionFlags, VectorXi, double, double, double,
                            double>(&ODEPhaseBase::addLUNormBound),
          ODEPhaseBase_addLUNormBound1);
  obj.def("addLUNormBound",
          py::overload_cast<PhaseRegionFlags, VectorXi, double, double, double>(
              &ODEPhaseBase::addLUNormBound),
          ODEPhaseBase_addLUNormBound2);
  obj.def("addLUNormBound",
          py::overload_cast<PhaseRegionFlags, VectorXi, double, double>(
              &ODEPhaseBase::addLUNormBound),
          ODEPhaseBase_addLUNormBound3);

  obj.def("addLUNormBound",
      py::overload_cast<std::string, VectorXi, double, double, double,
      double>(&ODEPhaseBase::addLUNormBound),
      ODEPhaseBase_addLUNormBound1);
  obj.def("addLUNormBound",
      py::overload_cast<std::string, VectorXi, double, double, double>(
          &ODEPhaseBase::addLUNormBound),
      ODEPhaseBase_addLUNormBound2);
  obj.def("addLUNormBound",
      py::overload_cast<std::string, VectorXi, double, double>(
          &ODEPhaseBase::addLUNormBound),
      ODEPhaseBase_addLUNormBound3);

  //////////////////////////////////////////////////////////////

  obj.def("addLowerSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double>(
          &ODEPhaseBase::addLowerSquaredNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addLowerSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double>(
          &ODEPhaseBase::addLowerSquaredNormBound),
      ODEPhaseBase_addUpperVarBound2);

  obj.def("addLowerSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double, double>(
          &ODEPhaseBase::addLowerSquaredNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addLowerSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double>(
          &ODEPhaseBase::addLowerSquaredNormBound),
      ODEPhaseBase_addUpperVarBound2);


  obj.def("addUpperSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double>(
          &ODEPhaseBase::addUpperSquaredNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addUpperSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double>(
          &ODEPhaseBase::addUpperSquaredNormBound),
      ODEPhaseBase_addUpperVarBound2);

  obj.def("addUpperSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double, double>(
          &ODEPhaseBase::addUpperSquaredNormBound),
      ODEPhaseBase_addUpperVarBound1);
  obj.def("addUpperSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double>(
          &ODEPhaseBase::addUpperSquaredNormBound),
      ODEPhaseBase_addUpperVarBound2);


  obj.def("addLUSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double, double,
      double>(&ODEPhaseBase::addLUSquaredNormBound),
      ODEPhaseBase_addLUNormBound1);
  obj.def("addLUSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double, double>(
          &ODEPhaseBase::addLUSquaredNormBound),
      ODEPhaseBase_addLUNormBound2);
  obj.def("addLUSquaredNormBound",
      py::overload_cast<PhaseRegionFlags, VectorXi, double, double>(
          &ODEPhaseBase::addLUSquaredNormBound),
      ODEPhaseBase_addLUNormBound3);

  obj.def("addLUSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double, double, double,
      double>(&ODEPhaseBase::addLUSquaredNormBound),
      ODEPhaseBase_addLUNormBound1);
  obj.def("addLUSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double, double, double>(
          &ODEPhaseBase::addLUSquaredNormBound),
      ODEPhaseBase_addLUNormBound2);
  obj.def("addLUSquaredNormBound",
      py::overload_cast<std::string, VectorXi, double, double>(
          &ODEPhaseBase::addLUSquaredNormBound),
      ODEPhaseBase_addLUNormBound3);


  //////////////////////////////////////////////////////////////

  obj.def("addLowerFuncBound",
          py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
                            double, double>(&ODEPhaseBase::addLowerFuncBound),
          ODEPhaseBase_addLowerFuncBound1);
  obj.def("addUpperFuncBound",
          py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
                            double, double>(&ODEPhaseBase::addUpperFuncBound),
          ODEPhaseBase_addUpperFuncBound2);

  obj.def("addLowerFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double, double>(&ODEPhaseBase::addLowerFuncBound),
      ODEPhaseBase_addLowerFuncBound1);
  obj.def("addUpperFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double, double>(&ODEPhaseBase::addUpperFuncBound),
      ODEPhaseBase_addUpperFuncBound2);



  obj.def("addLowerFuncBound",
      py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
      double>(&ODEPhaseBase::addLowerFuncBound),
      ODEPhaseBase_addLowerFuncBound1);
  obj.def("addUpperFuncBound",
      py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
      double>(&ODEPhaseBase::addUpperFuncBound),
      ODEPhaseBase_addUpperFuncBound2);

  obj.def("addLowerFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double>(&ODEPhaseBase::addLowerFuncBound),
      ODEPhaseBase_addLowerFuncBound1);
  obj.def("addUpperFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double>(&ODEPhaseBase::addUpperFuncBound),
      ODEPhaseBase_addUpperFuncBound2);



  obj.def("addLUFuncBound",
      py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
      double, double,double,double>(&ODEPhaseBase::addLUFuncBound));
  obj.def("addLUFuncBound",
      py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
      double, double,double>(&ODEPhaseBase::addLUFuncBound));
  obj.def("addLUFuncBound",
      py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,
      double, double>(&ODEPhaseBase::addLUFuncBound));

  obj.def("addLUFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double, double, double, double>(&ODEPhaseBase::addLUFuncBound));
  obj.def("addLUFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double, double, double>(&ODEPhaseBase::addLUFuncBound));
  obj.def("addLUFuncBound",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi,
      double, double>(&ODEPhaseBase::addLUFuncBound));


  //////////////////////////////////////////////////////////////


  obj.def("addLowerDeltaVarBound",
          py::overload_cast<PhaseRegionFlags, int, double, double>(
              &ODEPhaseBase::addLowerDeltaVarBound),
          ODEPhaseBase_addLowerDeltaVarBound1);
  obj.def("addLowerDeltaVarBound",
          py::overload_cast<int, double, double>(
              &ODEPhaseBase::addLowerDeltaVarBound),
          ODEPhaseBase_addLowerDeltaVarBound2);
  obj.def("addLowerDeltaVarBound",
          py::overload_cast<int, double>(&ODEPhaseBase::addLowerDeltaVarBound),
          ODEPhaseBase_addLowerDeltaVarBound3);

  obj.def(
      "addLowerDeltaTimeBound",
      py::overload_cast<double, double>(&ODEPhaseBase::addLowerDeltaTimeBound),
      ODEPhaseBase_addLowerDeltaTimeBound1);
  obj.def("addLowerDeltaTimeBound",
          py::overload_cast<double>(&ODEPhaseBase::addLowerDeltaTimeBound),
          ODEPhaseBase_addLowerDeltaTimeBound2);

  obj.def("addUpperDeltaVarBound",
          py::overload_cast<PhaseRegionFlags, int, double, double>(
              &ODEPhaseBase::addUpperDeltaVarBound),
          ODEPhaseBase_addUpperDeltaVarBound1);
  obj.def("addUpperDeltaVarBound",
          py::overload_cast<int, double, double>(
              &ODEPhaseBase::addUpperDeltaVarBound),
          ODEPhaseBase_addUpperDeltaVarBound2);
  obj.def("addUpperDeltaVarBound",
          py::overload_cast<int, double>(&ODEPhaseBase::addUpperDeltaVarBound),
          ODEPhaseBase_addUpperDeltaVarBound3);
  obj.def(
      "addUpperDeltaTimeBound",
      py::overload_cast<double, double>(&ODEPhaseBase::addUpperDeltaTimeBound),
      ODEPhaseBase_addUpperDeltaTimeBound1);
  obj.def("addUpperDeltaTimeBound",
          py::overload_cast<double>(&ODEPhaseBase::addUpperDeltaTimeBound),
          ODEPhaseBase_addUpperDeltaTimeBound2);

  ////////////////////////////////////////////////////////////////////////////
  obj.def("addStateObjective",
          py::overload_cast<StateObjective>(&ODEPhaseBase::addStateObjective),
          ODEPhaseBase_addStateObjective);

  obj.def("addStateObjective",
      py::overload_cast<PhaseRegionFlags,ScalarFunctionalX,VectorXi>(&ODEPhaseBase::addStateObjective),
      ODEPhaseBase_addStateObjective);

  obj.def("addStateObjective",
      py::overload_cast<PhaseRegionFlags, ScalarFunctionalX, VectorXi,VectorXi,VectorXi>(&ODEPhaseBase::addStateObjective),
      ODEPhaseBase_addStateObjective);

  obj.def("addStateObjective",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi>(&ODEPhaseBase::addStateObjective),
      ODEPhaseBase_addStateObjective);

  obj.def("addStateObjective",
      py::overload_cast<std::string, ScalarFunctionalX, VectorXi, VectorXi, VectorXi>(&ODEPhaseBase::addStateObjective),
      ODEPhaseBase_addStateObjective);


  obj.def("addValueObjective", py::overload_cast<PhaseRegionFlags, int, double>(& ODEPhaseBase::addValueObjective),
          ODEPhaseBase_addValueObjective);

  obj.def("addValueObjective", py::overload_cast<std::string, int, double>(&ODEPhaseBase::addValueObjective),
      ODEPhaseBase_addValueObjective);


  obj.def("addDeltaVarObjective", &ODEPhaseBase::addDeltaVarObjective,
          ODEPhaseBase_addDeltaVarObjective);
  obj.def("addDeltaTimeObjective", &ODEPhaseBase::addDeltaTimeObjective,
          ODEPhaseBase_addDeltaTimeObjective);

  obj.def(
      "addIntegralObjective",
      py::overload_cast<StateObjective>(&ODEPhaseBase::addIntegralObjective),
      ODEPhaseBase_addIntegralObjective1);
  obj.def("addIntegralObjective",
          py::overload_cast<ScalarFunctionalX, VectorXi>(
              &ODEPhaseBase::addIntegralObjective),
          ODEPhaseBase_addIntegralObjective2);
  obj.def("addIntegralObjective",
      py::overload_cast<ScalarFunctionalX, VectorXi,VectorXi,VectorXi>(
          &ODEPhaseBase::addIntegralObjective),
      ODEPhaseBase_addIntegralObjective2);
  ///////////////////////////////////////////////////////////////////////////////
  obj.def("addIntegralParamFunction",
          py::overload_cast<StateObjective, int>(
              &ODEPhaseBase::addIntegralParamFunction),
          ODEPhaseBase_addIntegralParamFunction1);
  obj.def("addIntegralParamFunction",
          py::overload_cast<ScalarFunctionalX, VectorXi, int>(
              &ODEPhaseBase::addIntegralParamFunction),
          ODEPhaseBase_addIntegralParamFunction2);
  obj.def("addIntegralParamFunction",
      py::overload_cast<ScalarFunctionalX, VectorXi,VectorXi,VectorXi, int>(
          &ODEPhaseBase::addIntegralParamFunction),
      ODEPhaseBase_addIntegralParamFunction2);
  


  
}
