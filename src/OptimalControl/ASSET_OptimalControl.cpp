#include "ASSET_OptimalControl.h"
#include "ODEArguments.h"
#include "Integrators/Integrator.h"

namespace ASSET {


 



void OptimalControlBuild(FunctionRegistry& reg, py::module& m) {
  auto & oc = reg.getOptimalControlModule();
  RKFlagsBuild(oc);
  OCPFlagsBuild(oc);


  StateFunction<GenericFunction<-1, -1>>::Build(oc,"StateConstraint");
  StateFunction<GenericFunction<-1, 1>>::Build(oc,"StateObjective");
  LinkFunction<GenericFunction<-1, -1>>::Build(oc,"LinkConstraint");
  LinkFunction<GenericFunction<-1, 1>>::Build(oc, "LinkObjective");
  ODEPhaseBase::Build(oc);
  OptimalControlProblem::Build(oc);
  LGLInterpTable::Build(oc);

  reg.Build_Register<InterpFunction<-1>>(oc, "InterpFunction");
  reg.Build_Register<InterpFunction<1>>(oc, "InterpFunction_1");
  reg.Build_Register<InterpFunction<3>>(oc, "InterpFunction_3");
  reg.Build_Register<InterpFunction<6>>(oc, "InterpFunction_6");

  FDDerivArbitrary<Eigen::VectorXd>::Build(oc,"FiniteDiffTable");
  ODEArguments<-1, -1, -1>::Build(oc, "ODEArguments");


  //Integrator< GenericODE<GenericFunction<-1, -1>, -1, 0, 0>>::Build(oc, "TestIntegratorX");
  //Integrator< GenericODE<GenericFunction<-1, -1>, 6, 0, 0>>::Build(oc, "TestIntegrator6");

  //Integrator< GenericODE<GenericFunction<-1, -1>, -1, -1, 0>>::Build(oc, "TestIntegratorXU");

  //reg.Build_Register<Integrator< GenericODE<GenericFunction<-1, -1>, -1, -1, 0>>>(oc, "TestIntegratorXU");
  //reg.Build_Register<Integrator< GenericODE<GenericFunction<-1, -1>, -1, -1, -1>>>(oc, "TestIntegratorXUP");



  GenericODESBuildPart1(reg, oc);
  GenericODESBuildPart2(reg, oc);
  GenericODESBuildPart3(reg, oc);
  GenericODESBuildPart4(reg, oc);
  GenericODESBuildPart5(reg, oc);
  GenericODESBuildPart6(reg, oc);

}

}  // namespace ASSET
