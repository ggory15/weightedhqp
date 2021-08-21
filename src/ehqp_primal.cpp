#include "weighted_hqp/ehqp_primal.hpp"
#include <assert.h>
#include <Eigen/QR>
using namespace std;

#define DEBUG_QP
namespace hcod{
    Ehqp_primal::Ehqp_primal(const std::vector<h_structure> &h, const Eigen::MatrixXd & Y)
    : h_(h), Y_(Y){
        _isweighted = false;
    } 
    Ehqp_primal::Ehqp_primal(const std::vector<h_structure> &h)
    : h_(h){
        _isweighted = true;
    } 

    void Ehqp_primal::compute(){
        if (!_isweighted){
            for (int i=0; i<p_; i++){
                h_structure hk = h_[i];
                
                if (hk.r > 0){
                    Eigen::VectorXi im1_idx = Eigen::VectorXi::LinSpaced(hk.m-hk.n, hk.n, hk.m-1);
                    L_ = hk.H(im1_idx, Eigen::VectorXi::LinSpaced(hk.ra-hk.rp, hk.rp, hk.ra-1));
                    if (hk.rp > 0)
                        M1_ = hk.H(im1_idx, Eigen::VectorXi::LinSpaced(hk.rp, 0, hk.rp-1));
                    W1_ = hk.W.block(0, 0, hk.m, hk.m);
                    b_ = hk.b(hk.activeb, 0);

                    if (hk.rp > 0)
                        e_ = W1_.transpose() * b_ - M1_ * y_;
                    else
                        e_ = W1_.transpose() * b_;
                    
                    y_.segment(hk.rp, hk.ra-hk.rp) = L_.completeOrthogonalDecomposition().pseudoInverse() * e_;
                    
                }
            }
            x_ = Y_.leftCols(h_[p_-1].ra) * y_;
        }
        else{
            for (int i=0; i<p_; i++){
                h_structure hk = h_[i], hj;
                if (i>0)
                    hj = h_[i-1];
                if (hk.r > 0){
                    Eigen::VectorXi im1_idx = Eigen::VectorXi::LinSpaced(hk.m-hk.n, hk.n, hk.m-1);
                    L_ = hk.H(im1_idx, Eigen::VectorXi::LinSpaced(hk.ra-hk.rp, hk.rp, hk.ra-1));
                    if (hk.rp > 0)
                        M1_ = hk.H(im1_idx, Eigen::VectorXi::LinSpaced(hk.rp, 0, hk.rp-1));
                    W1_ = hk.W(hk.iw, im1_idx);
                    b_ = hk.b(hk.activeb, 0); //shuld be check 0

// #ifdef DEBUG_QP
//     cout << "hk.H " << hk.H << endl; 
//     cout << "M1 " << M1_ << endl;
//     cout << "W1 " << W1_ << endl;
//     cout << "b_ " << b_.transpose() << endl;
//     getchar();
// #endif
                    if (i > 0){
                        e_ = W1_.transpose() * b_ - W1_.transpose() * hk.A(hk.active, Eigen::VectorXi::LinSpaced(hk.A.cols(), 0, hk.A.cols()-1)) * hj.sol;
                        // cout << "b_" << b_.transpose() << endl; 
                        // cout << "hk.A_act" << hk.A(hk.active, Eigen::VectorXi::LinSpaced(hk.A.cols(), 0, hk.A.cols()-1)) << endl; 
                        // cout << "hj.sol" << hj.sol.transpose() << endl; 
                        // cout << "e_" << e_.transpose() << endl; 
                    }
                    else{
                        e_ = W1_.transpose() * b_;
                    }
                    y_.segment(hk.rp, hk.ra-hk.rp) = L_.completeOrthogonalDecomposition().pseudoInverse() * e_;
                    // cout << "L" << L_ << endl;
                    // cout << "y" << y_.transpose() << endl; 
                }

                if (i == 0)
                    hk.sol = hk.Wk * hk.Y.block(0, hk.rp, hk.Y.rows(), hk.ra-hk.rp) * y_.segment(hk.rp, hk.ra-hk.rp);
                else
                    hk.sol = hj.sol + hk.Wk * hk.Y.block(0, hk.rp, hk.Y.rows(), hk.ra-hk.rp) * y_.segment(hk.rp, hk.ra-hk.rp);
                // cout << "sol" << hk.sol.transpose() << endl;
                // getchar();
                h_[i] = hk;
            }
            x_ = h_[p_-1].sol;
        }
    }
}