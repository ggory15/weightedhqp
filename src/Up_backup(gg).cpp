#include "weighted_hqp/Up.hpp"

#include <assert.h>
#include <numeric>
#include <Eigen/QR>

using namespace std;

//#define DEBUG
namespace hcod{
    bool Up::compute(const int kup, const int cup, const int bound, const std::vector<h_structure> & h, const Eigen::MatrixXd & Y, const bool & isweighted){
        Y_ = Y;
        _isweighted = isweighted;
        nh_ = h[0].A.cols();
        p_ = h.size();
        h_ = h;

        h_[kup].iw.conservativeResize(std::distance(h_[kup].iw.begin(), h_[kup].iw.end()+ 1));
        h_[kup].iw(h_[kup].iw.size() - 1) = h_[kup].fw(0);
        Eigen::VectorXi fw_tmp;
        fw_tmp = h_[kup].fw;
        h_[kup].fw = fw_tmp.tail(fw_tmp.size() - 1); //.conservativeResize(std::distance(h_[kup].fw.begin()+1, h_[kup].fw.end()));
       
        h_[kup].im.conservativeResize(std::distance(h_[kup].im.begin(), h_[kup].im.end()+ 1));
        h_[kup].im(h_[kup].im.size() - 1) = h_[kup].fm(0);
        fw_tmp = h_[kup].fm;  
        h_[kup].fm = fw_tmp.tail(fw_tmp.size() - 1);

        int idx_im = h_[kup].im(h_[kup].im.size() - 1);
        int idx_iw = h_[kup].iw(h_[kup].iw.size() - 1);
        h_[kup].m = h_[kup].m + 1;

        if (_isweighted){
            for (int i=kup+1; i<p_; i++)
            {
                h_[i].iwj[kup] = h_[kup].iw;
                h_[i].fwj[kup] = h_[kup].fw;
                h_[i].imj[kup] = h_[kup].im;
                h_[i].fmj[kup] = h_[kup].fm;
            
                if (h_[i].Hj[kup].rows() == 0){
                    h_[i].Hj[kup] = h_[i].Aj[kup].row(cup) * h_[i].Y;
                    h_[i].Wj[kup] = Eigen::MatrixXd::Identity(1,1);
                }
                else{
                    h_[i].Hj[kup].conservativeResize(h_[i].Hj[kup].rows() +1, h_[i].Hj[kup].cols());
                    h_[i].Hj[kup].row(idx_im) = h_[i].Aj[kup].row(cup) * h_[i].Y; 
                    h_[i].Wj[kup].conservativeResize(h_[i].Wj[kup].rows() +1, h_[i].Wj[kup].cols() + 1);
                    h_[i].Wj[kup].row(idx_iw).setZero();
                    h_[i].Wj[kup].col(idx_im).setZero();                        
                    h_[i].Wj[kup](idx_iw, idx_im) = 1.;
                }
                
                h_[i].mj[kup] = h_[kup].m;

            }
            // cout << "H gg " <<  cup << endl;
            h_[kup].H.row(idx_im) = h_[kup].AkWk.row(cup) * h_[kup].Y;
            // cout << "result " << h_[kup].AkWk.row(cup) * h_[kup].Y << endl;
        }
        else{
            h_[kup].H.row(idx_im) = h_[kup].A.row(cup) * Y_;
        }
        h_[kup].W.row(idx_iw).setZero();
        h_[kup].W.col(idx_im).setZero();
        h_[kup].W(idx_iw, idx_im) = 1.;
        
        h_[kup].active.conservativeResize(std::distance(h_[kup].active.begin(), h_[kup].active.end()+ 1));
        h_[kup].active(h_[kup].active.size() - 1) = cup;
        h_[kup].activeb.conservativeResize(std::distance(h_[kup].activeb.begin(), h_[kup].activeb.end()+ 1));
        h_[kup].activeb(h_[kup].activeb.size() - 1) = cup;

        if (bound == 2){
            h_[kup].activeb(h_[kup].activeb.size() - 1) += h_[kup].mmax;
        }
        
        h_[kup].bound(cup) = bound;
#ifdef DEBUG
if (_isweighted){
cout << "Aj" << h_[kup+1].Aj[kup].row(cup) << endl;
cout << "Wj " << h_[kup+1].Wj[kup] <<endl;
cout << "Hj " << h_[kup+1].Hj[kup] <<endl;
}
#endif
        Eigen::VectorXd flip_vec = h_[kup].H.row(idx_im).array().square();
        Eigen::VectorXd flip_vec_rev = flip_vec.reverse();
        Eigen::VectorXd csum = flip_vec_rev;
        std::partial_sum(flip_vec_rev.begin(), flip_vec_rev.end(), csum.begin(), std::plus<double>());
        int rup = nh_ - std::distance(csum.begin(), std::find_if(csum.begin(), csum.end(), [](const auto& x) { return x != 0; })) ;
        Eigen::VectorXi idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[kup].H.cols() ,0, h_[kup].H.cols()-1);
        Eigen::VectorXi im_tmp = h_[kup].im;
#ifdef DEBUG
cout << "dd" <<std::distance(csum.begin(), std::find_if(csum.begin(), csum.end(), [](const auto& x) { return x != 0; })) << endl;
cout << "rup " <<  rup << "  " <<csum.transpose() << endl;
cout << "h_[kup].ra " << h_[kup].ra << endl;
cout << "p_ " << p_ <<endl;
#endif
        if (rup <= h_[kup].ra){
            if (rup > h_[kup].rp){
                for (int i= rup - h_[kup].rp; i>=0; i--){         
                    given_->compute_rotation(h_[kup].H.col(h_[kup].rp + i)(h_[kup].im), h_[kup].n + i, h_[kup].m);
                    Wi_ = given_->getR().transpose();
#ifdef DEBUG
cout << "Wi_ " <<  Wi_ << endl;

#endif
                    Eigen::MatrixXd H_prev = h_[kup].H(h_[kup].im, idx_H_col_vec);
                    Eigen::MatrixXd W_prev = h_[kup].W(h_[kup].iw, h_[kup].im);
                    h_[kup].H(h_[kup].im, idx_H_col_vec) = Wi_ * H_prev;
                    h_[kup].W(h_[kup].iw, h_[kup].im) =  W_prev * Wi_.transpose();

#ifdef DEBUG
cout << "h_[kup].H(h_[kup].im, idx_H_col_vec) " <<  h_[kup].H(h_[kup].im, idx_H_col_vec) << endl;
cout << "h_[kup].W(h_[kup].iw, h_[kup].im) " <<  h_[kup].W(h_[kup].iw, h_[kup].im) << endl;
cout << "Wi_ " <<  Wi_ << endl;
#endif
                }                        
            }
            h_[kup].im(0) = im_tmp(h_[kup].im.size() - 1);
            for (int i=1; i<h_[kup].im.size(); i++)
                h_[kup].im(i) = im_tmp(i-1);
            h_[kup].n += 1;          
            
            if (!_isweighted)     
                return true;     
        }
        
        if (_isweighted){
            for (int j = kup +1; j<p_; j++){
                flip_vec = h_[j].Hj[kup].row(idx_im).array().square();
                flip_vec_rev = flip_vec.reverse();
                csum = flip_vec_rev;

                std::partial_sum(flip_vec_rev.begin(), flip_vec_rev.end(), csum.begin(), std::plus<double>());
                h_[j].rupj = nh_ - std::distance(csum.begin(), std::find_if(csum.begin(), csum.end(), [](const auto& x) { return x != 0; }));
                
                if (h_[j].rupj <= h_[kup].ra){
                    if (h_[j].rupj > h_[kup].rp){
                        for (int i=h_[j].rupj -h_[kup].rp; i>=0; i--){
                            given_->compute_rotation(h_[j].Hj[kup].col(h_[kup].rp + i)(h_[kup].im), h_[kup].n + i, h_[kup].m);
                            Wi_ = given_->getR().transpose();
                            h_[j].Hj[kup](h_[kup].im, idx_H_col_vec) = Wi_ * h_[kup].H(h_[kup].im, idx_H_col_vec);
                            h_[j].Wj[kup](h_[kup].iw, h_[kup].im) =  h_[kup].W(h_[kup].iw, h_[kup].im) * Wi_.transpose();
                        }
                    }
                    h_[j].imj[kup](0) = im_tmp(h_[kup].im.size() - 1);
                    for (int i=1; i<h_[kup].im.size(); i++)
                        h_[j].imj[kup](i) = im_tmp(i-1);
                    h_[j].nj[kup] = h_[kup].n + 1;        

                    if (j == p_-1){
                        return true;
                    }  
#ifdef DEBUG
cout << "h_[j].Hj[kup] " <<  h_[j].Hj[kup] << endl;
cout << " h_[j].Wj[kup] " <<   h_[j].Wj[kup] << endl;
cout << "h_[j].imj[kup] " <<  h_[j].imj[kup] << endl;
cout << "h_[j].nj[kup] " <<  h_[j].nj[kup] << endl;
#endif
                }
            }
        }

        Yup_.setIdentity(nh_, nh_);
        for (int i=rup-2; i>= h_[kup].ra; i--){ // check
#ifdef DEBUG
cout << "original " << h_[kup].H.row(h_[kup].im(h_[kup].im.size() -1)) << endl;
#endif
            given_->compute_rotation(h_[kup].H.row(h_[kup].im(h_[kup].im.size() -1)), i, i+1);
            Yi_ = given_->getR();     
            Eigen::MatrixXd H_prev = h_[kup].H.row(h_[kup].im(h_[kup].im.size() -1));
            h_[kup].H.row(h_[kup].im(h_[kup].im.size() -1)) = H_prev * Yi_;
                                    
            Yup_ = Yup_ * Yi_;

        }
#ifdef DEBUG
cout << "Yup_ " <<Yup_ << endl;
#endif
        if (_isweighted)
        {
            h_[kup].Y = h_[kup].Y * Yup_;
            for (int j = kup +1; j<p_; j++){
                h_[j].Yupj.setIdentity(nh_, nh_);
                for (int i=h_[j].rupj-2; i>=h_[kup].ra; i--){
                    given_->compute_rotation(h_[j].Hj[kup].row(h_[kup].im(h_[kup].im.size() -1)), i, i+1);
                    Yi_ = given_->getR();        
                    Eigen::MatrixXd Hj_prev = h_[j].Hj[kup].row(h_[kup].im(h_[kup].im.size() -1)); 
                    Eigen::MatrixXd Yupj_prev = h_[j].Yupj;
                    h_[j].Hj[kup].row(h_[kup].im(h_[kup].im.size() -1)) = Hj_prev * Yi_;                                            
                    h_[j].Yupj =  Yupj_prev * Yi_;
                }

                h_[j].rj[kup] = h_[kup].r + 1;
                h_[j].raj[kup] = h_[kup].ra + 1;
            }
        }

        h_[kup].r += 1;
        h_[kup].ra += 1;
#ifdef DEBUG
cout << "Yi_ " << Yi_ << endl;
if (_isweighted){
    cout <<"h_[j].Yupj "<<  h_[kup+1].Yupj << endl;
}
#endif

        if (!_isweighted)
        {
            for (int k=kup +1; k<p_; k++){
                Eigen::VectorXi idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[k].H.cols() ,0, h_[k].H.cols()-1);
                Eigen::MatrixXd H_prev = h_[k].H(h_[k].im, idx_H_col_vec);
                h_[k].H(h_[k].im, idx_H_col_vec) = H_prev * Yup_;
            
#ifdef DEBUG
cout << "h_[0].H(  " << h_[0].H  << endl;
cout << "h_[1].H(  " << h_[1].H  << endl;
#endif
                if (rup < h_[k].rp + 1){

                }
                else if (rup > h_[k].ra) {
                    h_[k].rp += 1;
                    h_[k].ra += 1;
                }
                else{
                    
                    int rdef = rup -  h_[k].rp;
                    for (int i = rdef; i>=2; i--){
                        given_->compute_rotation(h_[k].H.col(h_[k].rp + i - 1)(h_[k].im), h_[k].n + i -1, h_[k].n + rdef -1);
                        Wi_ = given_->getR().transpose();
                        Eigen::VectorXi idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[k].H.cols() ,0, h_[k].H.cols()-1);
                        Eigen::MatrixXd H_prev = h_[k].H(h_[k].im, idx_H_col_vec);
                        Eigen::MatrixXd W_prev = h_[k].W(h_[k].iw, h_[k].im);
                        h_[k].H(h_[k].im, idx_H_col_vec) = Wi_ * H_prev;
                        h_[k].W(h_[k].iw, h_[k].im) =  W_prev * Wi_.transpose();

#ifdef DEBUG
cout << "Wi_  " <<  Wi_ << endl;
cout << "h_[k].W(h_[k].iw, h_[k].im)  " << h_[k].W(h_[k].iw, h_[k].im)  << endl;
#endif
                    }
                    h_[k].rp += 1;
                    h_[k].r -= 1;
                    
                    h_[k].im.resize(h_[k].m);
                    Eigen::VectorXi im_tmp = h_[k].im;
                    h_[k].im(0) = im_tmp(h_[k].n + rdef - 1);
                    for (int i =0 ; i<h_[k].n; i++)
                        h_[k].im(i+1) = im_tmp(i);
                    for (int i =h_[kup].n; i<h_[k].n + rdef - 1; i++)
                        h_[k].im(i+1) = im_tmp(i);
                    for (int i =h_[k].n + rdef; i<h_[k].m; i++) 
                        h_[k].im(i) = im_tmp(i);
                    h_[k].n += 1;         

                                    
                }
#ifdef DEBUG
cout << "H_  " << h_[k].H  << endl;

#endif 
            }
            Y_ = Y_ * Yup_;
        }
        else
        {
             
            for (int k=kup +1; k<p_; k++){   
                      
                for (int j=kup+1; j < k-1; j++){
                    cout << "h_[k].rupj" << h_[k].rupj <<  h_[k].rpj[j] << endl;
                    getchar();
                    Eigen::VectorXi idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[j].Hj[j].cols() ,0, h_[j].Hj[j].cols()-1);       
                    Eigen::MatrixXd H_prev = h_[j].Hj[j](h_[j].imj[j], idx_H_col_vec);       
                    h_[j].Hj[j](h_[j].imj[j], idx_H_col_vec) = H_prev  * h_[k].Yupj;
                    if (h_[k].rupj < h_[k].rpj[j]+1){

                    }
                    else if (h_[k].rupj > h_[k].raj[j]){

                    }
                    else{
                       
                        int rdef = h_[k].rupj -  h_[k].rpj[j];
                        for (int i = rdef; i>=2; i--){
                            given_->compute_rotation(h_[j].Hj[j].col(h_[k].rpj[j] + i-1)(h_[k].imj[j]), h_[k].nj[j] + i-1, h_[k].nj[j] + rdef-1); //check
                            Wi_ = given_->getR().transpose();
                            idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[j].Hj[j].cols() ,0, h_[j].Hj[j].cols()-1);
                            Eigen::MatrixXd H_prev = h_[j].Hj[j](h_[k].imj[j], idx_H_col_vec);       
                            Eigen::MatrixXd W_prev = h_[k].Wj[j](h_[k].iwj[j], h_[k].imj[j]);       
                            h_[j].Hj[j](h_[k].im, idx_H_col_vec) = Wi_ * H_prev;
                            h_[k].Wj[j](h_[k].iwj[j], h_[k].imj[j]) =  W_prev * Wi_.transpose();
                        }
                        h_[k].imj[j].resize(-1 + h_[k].mj[j]);
                        h_[k].imj[j](0) = h_[k].nj[j] + rdef;
                    }
                } 
                idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[k].H.cols() ,0, h_[k].H.cols()-1);
                Eigen::MatrixXd H_prev = h_[k].H(h_[k].im, idx_H_col_vec);  
                h_[k].H(h_[k].im, idx_H_col_vec) = h_[k].H(h_[k].im, idx_H_col_vec) * h_[k].Yupj;

#ifdef DEBUG
cout << " h_[k].Yup  " <<  h_[k].Yupj  << endl;
cout << "h_[k].H  " << h_[k].H  << endl;
#endif
                if (h_[k].rupj < h_[k].rp + 1){

                }
                else if (h_[k].rupj > h_[k].ra) {

#ifdef DEBUG
cout <<" case 2" << endl;
#endif             
                    int rp = h_[k].rp;
                    int ra = h_[k].ra;
                    h_[k].rp = rp + 1;
                    h_[k].ra = ra + 1;

                    for (int i=k+1; i<p_; i++){
                        h_[i].rpj[k] = rp + 1;
                        h_[i].raj[k] = ra + 1;
                    }
                }
                else{

       
                
                    int rdef = h_[k].rupj -  h_[k].rp;
                    for (int i = rdef; i>=2; i--){
                        given_->compute_rotation(h_[k].H.col(h_[k].rp + i)(h_[k].im), h_[k].n + i, h_[k].n + rdef);
                        Wi_ = given_->getR().transpose();
                        Eigen::VectorXi idx_H_col_vec = Eigen::VectorXi::LinSpaced(h_[k].H.cols() ,0, h_[k].H.cols()-1);
                        Eigen::MatrixXd H_prev = h_[k].H(h_[k].im, idx_H_col_vec);       
                        Eigen::MatrixXd W_prev = h_[k].W(h_[k].iw, h_[k].im);  

                        h_[k].H(h_[k].im, idx_H_col_vec) = Wi_ * H_prev;
                        h_[k].W(h_[k].iw, h_[k].im) = W_prev * Wi_.transpose();
                    }

                    int rp_tmp = h_[k].rp;
                    int r_tmp = h_[k].r;

                    h_[k].rp = rp_tmp +1;
                    h_[k].r = r_tmp - 1;
                    

                    
                    h_[k].im.resize(h_[k].m);
    
                    Eigen::VectorXi im_tmp = h_[k].im;
                    h_[k].im(0) = im_tmp(h_[k].n + rdef - 1);
                    for (int i =0 ; i<h_[k].n; i++)
                        h_[k].im(i+1) = im_tmp(i);
                    for (int i =h_[kup].n; i<h_[k].n + rdef - 1; i++)
                        h_[k].im(i+1) = im_tmp(i);
                    for (int i =h_[k].n + rdef; i<h_[k].m; i++) {
                        h_[k].im(i) = im_tmp(i);
                    }

                    for (int i=k+1; i<p_; i++){
                        h_[i].rpj[k] = rp_tmp + 1;
                        h_[i].rj[k] = r_tmp + 1;
                        h_[i].nj[k] = h_[k].n +1;
                    }   
                    // cout << "h_[k].n  " << h_[k].n  << endl;
                    // cout << "hk im" << h_[k].im.transpose() << endl;
                    h_[k].n += 1;
                    // getchar();
             
                }
                Eigen::MatrixXd Y_prev = h_[k].Y;
                h_[k].Y = Y_prev* h_[k].Yupj;

#ifdef DEBUG
cout <<" case 3" << endl;
cout <<" h_[k].Y" << h_[k].Y << endl;
#endif      
            }
            
#ifdef DEBUG
cout << "h_[0].H(  " << h_[0].H  << endl;
cout << "h_[1].H(  " << h_[1].H  << endl;
cout << "  " << endl;
cout << "  " << endl;
cout << "  " << endl;
cout << "  " << endl;
cout << "  " << endl;
cout << "  " << endl;
getchar();

#endif                
        }

#ifdef DEBUG
cout << "Y_  " << Y_  << endl;
#endif                  

        return true;
    }
}