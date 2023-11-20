pipeline {
  agent {
     node {
        label 'CentOS9_x86_64'
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
      emailext to: "cjw@ucar.edu janine@ucar.edu cdewerd@ucar.edu",
      subject: "Jenkinsfile aircraft_config_edit build failed",
      body: "See console output attached",
      attachLog: true
    }
  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '5'))
  }
}
