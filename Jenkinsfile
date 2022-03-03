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
    stage('Build') {
      steps {
        sh 'git submodule update --init --recursive'
        sh 'scons'
      }
    }
  }
  post {
    failure {
      mail(to: 'cjw@ucar.edu cdewerd@ucar.edu janine@ucar.edu, taylort@ucar.edu', subject: 'aircraft_config_edit Jenkins build failed', body: 'aircraft_config_edit Jenkins build failed')
    }
  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '5'))
  }
}
