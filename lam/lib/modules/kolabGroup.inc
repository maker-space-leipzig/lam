<?php

use LAM\TYPES\TypeManager;

/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2013 - 2024  Roland Gruber

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * Manages Kolab group accounts.
 *
 * @package modules
 * @author Roland Gruber
 */

/**
 * Manages Kolab group accounts.
 *
 * @package modules
 */
class kolabGroup extends baseModule {

	/** cache for mail attribute */
	private $mailCache;

	/**
	 * Creates a new kolabGroup object.
	 *
	 * @param string $scope account type (user, group, host)
	 */
	function __construct($scope) {
		// call parent constructor
		parent::__construct($scope);
		$this->autoAddObjectClasses = false;
	}

	/**
	 * Returns true if this module can manage accounts of the current type, otherwise false.
	 *
	 * @return boolean true if module fits
	 */
	public function can_manage() {
		return in_array($this->get_scope(), ['group']);
	}

	/**
	 * Returns meta data that is interpreted by parent class
	 *
	 * @return array array with meta data
	 *
	 * @see baseModule::get_metaData()
	 */
	function get_metaData() {
		$return = [];
		// icon
		$return['icon'] = 'kolab.png';
		// alias name
		$return["alias"] = _("Kolab");
		// module dependencies
		$return['dependencies'] = ['depends' => [], 'conflicts' => []];
		// managed object classes
		$return['objectClasses'] = ['kolabGroupOfUniqueNames'];
		// managed attributes
		$return['attributes'] = ['kolabAllowSMTPRecipient', 'kolabAllowSMTPSender'];
		// help Entries
		$return['help'] = [
			'mail' => [
				"Headline" => _("Email address"), 'attr' => 'mail',
				"Text" => _("The list's email address.")
			],
			'mailList' => [
				"Headline" => _("Email address"), 'attr' => 'mail',
				"Text" => _("The list's email address.") . ' ' . _("Multiple values are separated by semicolon.")
			],
			'kolabAllowSMTPRecipient' => [
				"Headline" => _('Allowed recipients'), 'attr' => 'kolabAllowSMTPRecipient',
				"Text" => _('Describes the allowed or disallowed SMTP recipient addresses for mail sent by this account (e.g. "domain.tld" or "-user@domain.tld").')
			],
			'kolabAllowSMTPRecipientList' => [
				"Headline" => _('Allowed recipients'), 'attr' => 'kolabAllowSMTPRecipient',
				"Text" => _('Describes the allowed or disallowed SMTP recipient addresses for mail sent by this account (e.g. "domain.tld" or "-user@domain.tld").')
					. ' ' . _("Multiple values are separated by semicolon.")
			],
			'kolabAllowSMTPSender' => [
				"Headline" => _('Allowed senders'), 'attr' => 'kolabAllowSMTPSender',
				"Text" => _('Describes the allowed or disallowed SMTP addresses sending mail to this account (e.g. "domain.tld" or "-user@domain.tld").')
			],
			'kolabAllowSMTPSenderList' => [
				"Headline" => _('Allowed senders'), 'attr' => 'kolabAllowSMTPSender',
				"Text" => _('Describes the allowed or disallowed SMTP addresses sending mail to this account (e.g. "domain.tld" or "-user@domain.tld").')
					. ' ' . _("Multiple values are separated by semicolon.")
			],
			'autoAdd' => [
				"Headline" => _("Automatically add this extension"),
				"Text" => _("This will enable the extension automatically if this profile is loaded.")
			],
			'addExtension' => [
				"Headline" => _("Add Kolab extension"),
				"Text" => _("Adds this Kolab extension.")
			],
		];
		// profile options
		$profileContainer = new htmlResponsiveRow();
		$profileContainer->add(new htmlResponsiveInputCheckbox('kolabGroup_addExt', false, _('Automatically add this extension'), 'autoAdd'), 12);
		$return['profile_options'] = $profileContainer;
		// upload fields
		$return['upload_columns'] = [
			[
				'name' => 'kolabGroup_addExtension',
				'description' => _('Add Kolab extension'),
				'help' => 'addExtension',
				'example' => 'false',
				'default' => 'false',
				'values' => 'true, false'
			],
			[
				'name' => 'kolabGroup_kolabAllowSMTPRecipient',
				'description' => _('Allowed recipients'),
				'help' => 'kolabAllowSMTPRecipientList',
				'example' => '.com; -.net',
			],
			[
				'name' => 'kolabGroup_kolabAllowSMTPSender',
				'description' => _('Allowed senders'),
				'help' => 'kolabAllowSMTPSenderList',
				'example' => '.com; -.net',
			],
		];
		// available PDF fields
		$return['PDF_fields'] = [
			'kolabAllowSMTPRecipient' => _('Allowed recipients'),
			'kolabAllowSMTPSender' => _('Allowed senders'),
		];
		return $return;
	}

	/**
	 * This function fills the $messages variable with output messages from this module.
	 */
	function load_Messages() {
		$this->messages['mail'][0] = ['ERROR', _('Email address'), _('Please enter a valid email address!')];
		$this->messages['mail'][1] = ['ERROR', _('Account %s:') . ' kolabGroup_mail', _('Please enter a valid email address!')];
		$this->messages['mail'][2] = ['ERROR', _('Email address'), _('Email address already exists.')];
		$this->messages['mail'][3] = ['ERROR', _('Account %s:') . ' kolabGroup_mail', _('Email address already exists.')];
		$this->messages['kolabAllowSMTPRecipient'][0] = ['ERROR', _('Allowed recipients'), _('Please enter a valid recipient expression.')];
		$this->messages['kolabAllowSMTPRecipient'][1] = ['ERROR', _('Account %s:') . ' kolabGroup_kolabAllowSMTPRecipient', _('Please enter a valid recipient expression.')];
		$this->messages['kolabAllowSMTPSender'][0] = ['ERROR', _('Allowed senders'), _('Please enter a valid sender expression.')];
		$this->messages['kolabAllowSMTPSender'][1] = ['ERROR', _('Account %s:') . ' kolabGroup_kolabAllowSMTPSender', _('Please enter a valid sender expression.')];
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	function display_html_attributes() {
		$container = new htmlResponsiveRow();
		if (isset($this->attributes['objectClass']) && in_array('kolabGroupOfUniqueNames', $this->attributes['objectClass'])) {
			// mail
			if ($this->manageMail($this->getAccountContainer()->get_type()->getModules())) {
				$this->addSimpleInputTextField($container, 'mail', _('Email address'), true);
			}
			// allowed recipients
			$this->addMultiValueInputTextField($container, 'kolabAllowSMTPRecipient', _('Allowed recipients'));
			// allowed senders
			$this->addMultiValueInputTextField($container, 'kolabAllowSMTPSender', _('Allowed senders'));
		}
		else {
			// add button
			$container->add(new htmlButton('addObjectClass', _('Add Kolab extension')));
		}
		return $container;
	}

	/**
	 * Processes user input of the primary module page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	function process_attributes() {
		$errors = [];
		if (isset($_POST['addObjectClass'])) {
			$this->attributes['objectClass'][] = 'kolabGroupOfUniqueNames';
			return $errors;
		}
		if (isset($_POST['remObjectClass'])) {
			$this->attributes['objectClass'] = array_delete(['kolabGroupOfUniqueNames'], $this->attributes['objectClass']);
			$attrs = $this->meta['attributes'];
			foreach ($attrs as $name) {
				if (isset($this->attributes[$name])) {
					unset($this->attributes[$name]);
				}
			}
			return $errors;
		}
		// skip processing if object class is not set
		if (!isset($this->attributes['objectClass']) || !in_array('kolabGroupOfUniqueNames', $this->attributes['objectClass'])) {
			return $errors;
		}
		// mail
		if ($this->manageMail($this->getAccountContainer()->get_type()->getModules())) {
			if (!empty($_POST['mail'])) {
				$this->attributes['mail'][0] = $_POST['mail'];
				// check format
				if (!get_preg($_POST['mail'], 'email')) {
					$errors[] = $this->messages['mail'][0];
				}
				// check if unique
				if ($this->getAccountContainer()->isNewAccount || (!empty($this->orig['mail'][0]) && ($this->orig['mail'][0] != $this->attributes['mail'][0]))) {
					$this->loadMailCache();
					if (in_array_ignore_case($_POST['mail'], $this->mailCache)) {
						$errors[] = $this->messages['mail'][2];
					}
				}
			}
			elseif (isset($this->attributes['mail'])) {
				unset($this->attributes['mail']);
			}
		}
		// allowed recipients
		$this->processMultiValueInputTextField('kolabAllowSMTPRecipient', $errors, 'kolabEmailPrefix');
		// allowed senders
		$this->processMultiValueInputTextField('kolabAllowSMTPSender', $errors, 'kolabEmailPrefix');
		return $errors;
	}

	/**
	 * Returns a list of modifications which have to be made to the LDAP account.
	 *
	 * @return array list of modifications
	 * <br>This function returns an array with 3 entries:
	 * <br>array( DN1 ('add' => array($attr), 'remove' => array($attr), 'modify' => array($attr)), DN2 .... )
	 * <br>DN is the DN to change. It may be possible to change several DNs (e.g. create a new user and add him to some groups via attribute memberUid)
	 * <br>"add" are attributes which have to be added to LDAP entry
	 * <br>"remove" are attributes which have to be removed from LDAP entry
	 * <br>"modify" are attributes which have to been modified in LDAP entry
	 * <br>"info" are values with informational value (e.g. to be used later by pre/postModify actions)
	 */
	function save_attributes() {
		if (!in_array('kolabGroupOfUniqueNames', $this->attributes['objectClass']) && !in_array('kolabGroupOfUniqueNames', $this->orig['objectClass'])) {
			// skip saving if the extension was not added/modified
			return [];
		}
		return parent::save_attributes();
	}

	/**
	 * Loads the values of an account profile into internal variables.
	 *
	 * @param array $profile hash array with profile values (identifier => value)
	 */
	function load_profile($profile) {
		// profile mappings in meta data
		parent::load_profile($profile);
		// add extension
		if (isset($profile['kolabGroup_addExt'][0])
			&& ($profile['kolabGroup_addExt'][0] == "true")
			&& !in_array('kolabGroupOfUniqueNames', $this->attributes['objectClass'])) {
			$this->attributes['objectClass'][] = 'kolabGroupOfUniqueNames';
		}
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::getManagedAttributes()
	 */
	function get_uploadColumns($selectedModules, &$type) {
		$return = parent::get_uploadColumns($selectedModules, $type);
		if ($this->manageMail($selectedModules)) {
			$return[] = [
				'name' => 'kolabGroup_mail',
				'description' => _('Email address'),
				'help' => 'mailList',
				'example' => 'list@company.com',
				'required' => true,
				'unique' => true,
			];
		}
		return $return;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$messages = [];
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// check if extension should be added
			if (isset($rawAccounts[$i][$ids['kolabGroup_addExtension']]) && ($rawAccounts[$i][$ids['kolabGroup_addExtension']] != 'true')) {
				continue;
			}
			// add object classes
			if (!in_array('kolabGroupOfUniqueNames', $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = 'kolabGroupOfUniqueNames';
			}
			// mail
			if ($this->manageMail($selectedModules) && !empty($rawAccounts[$i][$ids['kolabGroup_mail']])) {
				if (get_preg($rawAccounts[$i][$ids['kolabGroup_mail']], 'email')) {
					$this->loadMailCache();
					if (!in_array_ignore_case(trim($rawAccounts[$i][$ids['kolabGroup_mail']]), $this->mailCache)) {
						$partialAccounts[$i]['mail'] = trim($rawAccounts[$i][$ids['kolabGroup_mail']]);
					}
					else {
						$errMsg = $this->messages['mail'][3];
						array_push($errMsg, [$i]);
						$messages[] = $errMsg;
					}
				}
				else {
					$errMsg = $this->messages['mail'][1];
					array_push($errMsg, [$i]);
					$messages[] = $errMsg;
				}
			}
			// allowed recipients
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabGroup_kolabAllowSMTPRecipient', 'kolabAllowSMTPRecipient', 'kolabEmailPrefix', $this->messages['kolabAllowSMTPRecipient'][1], $messages, '/;[ ]*/');
			// allowed senders
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabGroup_kolabAllowSMTPSender', 'kolabAllowSMTPSender', 'kolabEmailPrefix', $this->messages['kolabAllowSMTPSender'][1], $messages, '/;[ ]*/');
		}
		return $messages;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfFields()
	 */
	public function get_pdfFields($typeId) {
		$fields = parent::get_pdfFields($typeId);
		$typeManager = new TypeManager();
		$modules = $typeManager->getConfiguredType($typeId)->getModules();
		if ($this->manageMail($modules)) {
			$fields['mail'] = _('Email address');
		}
		return $fields;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	function get_pdfEntries($pdfKeys, $typeId) {
		$return = [];
		$this->addSimplePDFField($return, 'mail', _('Email address'));
		$this->addSimplePDFField($return, 'kolabAllowSMTPRecipient', _('Allowed recipients'));
		$this->addSimplePDFField($return, 'kolabAllowSMTPSender', _('Allowed senders'));
		return $return;
	}

	/**
	 * Returns if the mail attribute should be managed.
	 *
	 * @param string[] $modules account modules
	 * @return boolean manage mail attribute
	 */
	private function manageMail($modules) {
		return !in_array('qmailGroup', $modules);
	}

	/**
	 * Loads the list of email addresses into the cache.
	 */
	private function loadMailCache() {
		if ($this->mailCache != null) {
			return;
		}
		$results = searchLDAPByFilter('(mail=*)', ['mail'], [$this->get_scope()]);
		$this->mailCache = [];
		foreach ($results as $result) {
			if (isset($result['mail'][0])) {
				$this->mailCache[] = $result['mail'][0];
			}
		}
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::getManagedAttributes()
	 */
	public function getManagedAttributes($typeId) {
		$attrs = parent::getManagedAttributes($typeId);
		$typeManager = new TypeManager();
		if ($this->manageMail($typeManager->getConfiguredType($typeId)->getModules())) {
			$attrs[] = 'mail';
		}
		return $attrs;
	}

}


?>
