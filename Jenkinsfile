pipeline {
  agent {
     node {
        label 'RHEL7_x86_64'
        }
  }
  triggers {
  pollSCM('H/30 8-18 * * * ')
  }
  stages {
    stage('Checkout Scm') {
      steps {
        git 'eolJenkins:ncar/aircraft_config_edit'
      }
    }
    stage('Build') {
      steps {
        sh 'scons'
      }
    }
  }
  post {
    success {
      mail(to: 'cjw@ucar.edu cdewerd@ucar.edu janine@ucar.edu, taylort@ucar.edu', subject: 'aircraft_config_edit Jenkins build successful', body: 'aircraft_config_edit Jenkins build successful')
    }
  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '5'))
  }
}
