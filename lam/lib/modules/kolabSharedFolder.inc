<?php
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
 * Manages Kolab shared folders.
 *
 * @package modules
 * @author Roland Gruber
 */

use LAM\TYPES\ConfiguredType;

/**
 * Manages Kolab shared folders.
 *
 * @package modules
 */
class kolabSharedFolder extends baseModule {

	/** list of folder types (label => id) */
	private $folderTypes = [];

	/**
	 * Creates a new kolabSharedFolder object.
	 *
	 * @param string $scope account type (user, group, host)
	 */
	function __construct($scope) {
		// call parent constructor
		parent::__construct($scope);
		$this->folderTypes = [
			_('Shared mail folder') => 'mail',
		];
	}

	/**
	 * Returns true if this module can manage accounts of the current type, otherwise false.
	 *
	 * @return boolean true if module fits
	 */
	public function can_manage() {
		return in_array($this->get_scope(), ['kolabSharedFolderType']);
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
		$return["alias"] = _("Kolab shared folder");
		// this is a base module
		$return["is_base"] = true;
		// RDN attribute
		$return["RDN"] = ["cn" => "normal"];
		// module dependencies
		$return['dependencies'] = ['depends' => [], 'conflicts' => []];
		// LDAP filter
		$return["ldap_filter"] = ['or' => "(objectClass=kolabSharedFolder)"];
		// managed object classes
		$return['objectClasses'] = ['kolabSharedFolder', 'mailrecipient'];
		// managed attributes
		$return['attributes'] = ['cn', 'kolabAllowSMTPRecipient', 'kolabAllowSMTPSender', 'acl',
			'alias', 'kolabDelegate', 'kolabFolderType', 'kolabTargetFolder', 'mail'];
		// help Entries
		$return['help'] = [
			'cn' => [
				"Headline" => _("Name"), 'attr' => 'cn',
				"Text" => _("Please enter a name for this folder.")
			],
			'mail' => [
				"Headline" => _("Email address"), 'attr' => 'mail',
				"Text" => _("The folder's email address.")
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
			'delegate' => [
				"Headline" => _("Delegates"), 'attr' => 'kolabDelegate',
				"Text" => _("Delegates are allowed to act on behalf of the shared folder. This property is checked when using the Kolab smtp daemon (Postfix) to send emails.")
			],
			'delegateList' => [
				"Headline" => _("Delegates"), 'attr' => 'kolabDelegate',
				"Text" => _("This is a semi-colon separated list of delegates.")
			],
			'alias' => [
				"Headline" => _("Email alias"), 'attr' => 'alias',
				"Text" => _("Email alias for this account.")
			],
			'aliasList' => [
				"Headline" => _("Email alias list"), 'attr' => 'alias',
				"Text" => _("This is a comma separated list of eMail aliases.")
			],
			'kolabTargetFolder' => [
				"Headline" => _("Target IMAP folder"), 'attr' => 'kolabTargetFolder',
				"Text" => _("The folder on the server where the shared folder is located (e.g. shared/myfolder@example.com).")
			],
			'kolabFolderType' => [
				"Headline" => _("Type"), 'attr' => 'kolabFolderType',
				"Text" => _("Specifies the folder type (e.g. shared mail folder).")
			],
		];
		// upload fields
		$return['upload_columns'] = [
			[
				'name' => 'kolabSharedFolder_cn',
				'description' => _('Name'),
				'help' => 'cn',
				'example' => 'folder',
				'required' => true
			],
			[
				'name' => 'kolabSharedFolder_mail',
				'description' => _('Email address'),
				'help' => 'mail',
				'example' => _('user@company.com'),
				'required' => true
			],
			[
				'name' => 'kolabSharedFolder_kolabTargetFolder',
				'description' => _('Target IMAP folder'),
				'help' => 'kolabTargetFolder',
				'example' => 'shared/myfolder@example.com',
				'required' => true
			],
			[
				'name' => 'kolabSharedFolder_kolabFolderType',
				'description' => _('Type'),
				'help' => 'kolabFolderType',
				'example' => 'mail',
				'required' => true
			],
			[
				'name' => 'kolabSharedFolder_kolabAllowSMTPRecipient',
				'description' => _('Allowed recipients'),
				'help' => 'kolabAllowSMTPRecipientList',
				'example' => '.com; -.net',
			],
			[
				'name' => 'kolabSharedFolder_kolabAllowSMTPSender',
				'description' => _('Allowed senders'),
				'help' => 'kolabAllowSMTPSenderList',
				'example' => '.com; -.net',
			],
			[
				'name' => 'kolabSharedFolder_aliases',
				'description' => _('Email aliases'),
				'help' => 'aliasList',
				'example' => 'user@domain,user2@domain'
			],
			[
				'name' => 'kolabSharedFolder_delegates',
				'description' => _('Delegates'),
				'help' => 'delegateList',
				'example' => 'uid=user,ou=People,dc=example,dc=com;uid=user2,ou=People,dc=example,dc=com'
			],
		];
		// available PDF fields
		$return['PDF_fields'] = [
			'cn' => _('Name'),
			'kolabAllowSMTPRecipient' => _('Allowed recipients'),
			'kolabAllowSMTPSender' => _('Allowed senders'),
			'aliases' => _('Email aliases'),
			'delegate' => _('Delegates'),
			'mail' => _('Email address'),
			'kolabTargetFolder' => _('Target IMAP folder'),
			'kolabFolderType' => _('Type'),
		];
		return $return;
	}

	/**
	 * This function fills the $messages variable with output messages from this module.
	 */
	function load_Messages() {
		$this->messages['kolabAllowSMTPRecipient'][0] = ['ERROR', _('Allowed recipients'), _('Please enter a valid recipient expression.')];
		$this->messages['kolabAllowSMTPRecipient'][1] = ['ERROR', _('Account %s:') . ' kolabSharedFolder_kolabAllowSMTPRecipient', _('Please enter a valid recipient expression.')];
		$this->messages['kolabAllowSMTPSender'][0] = ['ERROR', _('Allowed senders'), _('Please enter a valid sender expression.')];
		$this->messages['kolabAllowSMTPSender'][1] = ['ERROR', _('Account %s:') . ' kolabSharedFolder_kolabAllowSMTPSender', _('Please enter a valid sender expression.')];
		$this->messages['alias'][0] = ['ERROR', _('Email alias is invalid!')];  // third array value is set dynamically
		$this->messages['alias'][1] = ['ERROR', _('Account %s:') . ' kolabSharedFolder_aliases', _('Email alias list has invalid format!')];
		$this->messages['delegate'][0] = ['ERROR', _('Account %s:') . ' kolabSharedFolder_delegates', _('Unknown delegate: %s')];
		$this->messages['mail'][0] = ['ERROR', _('Email address'), _('Please enter a valid email address!')];
		$this->messages['mail'][1] = ['ERROR', _('Account %s:') . ' kolabSharedFolder_mail', _('Please enter a valid email address!')];
		$this->messages['cn'][0] = ['ERROR', _('Name'), _('Please enter a name.')];
		$this->messages['kolabTargetFolder'][0] = ['ERROR', _('Target IMAP folder'), _('Please enter a target folder.')];
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	function display_html_attributes() {
		$container = new htmlResponsiveRow();
		// name
		$this->addSimpleInputTextField($container, 'cn', _('Name'), true);
		// mail
		$this->addSimpleInputTextField($container, 'mail', _('Email address'), true);
		// target folder
		$this->addSimpleInputTextField($container, 'kolabTargetFolder', _('Target IMAP folder'), true);
		// folder type
		$possibleTypes = $this->folderTypes;
		$selectedTypes = ['mail'];
		if (!empty($this->attributes['kolabFolderType'])) {
			$selectedTypes = $this->attributes['kolabFolderType'];
			if (!in_array($this->attributes['kolabFolderType'][0], $possibleTypes)) {
				$possibleTypes[$this->attributes['kolabFolderType'][0]] = $this->attributes['kolabFolderType'][0];
			}
		}
		$typeSelect = new htmlResponsiveSelect('kolabFolderType', $possibleTypes, $selectedTypes, _('Type'), 'kolabFolderType');
		$typeSelect->setHasDescriptiveElements(true);
		$container->add($typeSelect);
		$container->addVerticalSpacer('1rem');
		// allowed recipients
		$this->addMultiValueInputTextField($container, 'kolabAllowSMTPRecipient', _('Allowed recipients'));
		// allowed senders
		$this->addMultiValueInputTextField($container, 'kolabAllowSMTPSender', _('Allowed senders'));
		// mail aliases
		$container->add(new htmlSubTitle(_('Email aliases')));
		$this->addMultiValueInputTextField($container, 'alias', null);
		// delegates
		$delegatesData = searchLDAPByAttribute('mail', '*', 'inetOrgPerson', ['dn'], ['user']);
		$delegates = [];
		for ($i = 0; $i < sizeof($delegatesData); $i++) {
			$delegates[getAbstractDN($delegatesData[$i]['dn'])] = $delegatesData[$i]['dn'];
		}
		uksort($delegates, 'compareDN');
		$container->add(new htmlSubTitle(_('Delegates')));
		$delegatesContainer = new htmlTable();
		if (isset($this->attributes['kolabDelegate'])) {
			for ($i = 0; $i < sizeof($this->attributes['kolabDelegate']); $i++) {
				$delegateSelect = new htmlSelect('delegate' . $i, $delegates, [$this->attributes['kolabDelegate'][$i]]);
				$delegateSelect->setHasDescriptiveElements(true);
				$delegateSelect->setSortElements(false);
				$delegatesContainer->addElement($delegateSelect);
				$delegatesContainer->addElement(new htmlButton('delDelegate' . $i, 'del.svg', true));
				if ($i == 0) {
					$delegatesContainer->addElement(new htmlHelpLink('delegate'));
				}
				$delegatesContainer->addNewLine();
			}
		}
		// input box for new delegate
		$delegatesContainer->addElement(new htmlSelect('delegate', $delegates));
		$delegatesContainer->addElement(new htmlButton('addDelegate', 'add.svg', true));
		if (empty($this->attributes['kolabDelegate'])) {
			$delegatesContainer->addElement(new htmlHelpLink('delegate'));
		}
		$container->add($delegatesContainer);
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
		// cn
		$this->attributes['cn'][0] = $_POST['cn'];
		if (empty($_POST['cn'])) {
			$errors[] = $this->messages['cn'][0];
		}
		// mail
		$this->attributes['mail'][0] = $_POST['mail'];
		if (empty($_POST['mail']) || !get_preg($_POST['mail'], 'email')) {
			$errors[] = $this->messages['mail'][0];
		}
		// target folder
		$this->attributes['kolabTargetFolder'][0] = $_POST['kolabTargetFolder'];
		if (empty($_POST['kolabTargetFolder'])) {
			$errors[] = $this->messages['kolabTargetFolder'][0];
		}
		// folder type
		$this->attributes['kolabFolderType'][0] = $_POST['kolabFolderType'];
		// allowed recipients
		$this->processMultiValueInputTextField('kolabAllowSMTPRecipient', $errors, 'kolabEmailPrefix');
		// allowed senders
		$this->processMultiValueInputTextField('kolabAllowSMTPSender', $errors, 'kolabEmailPrefix');
		// mail aliases
		$this->processMultiValueInputTextField('alias', $errors, 'email');
		// check old delegates
		$this->attributes['kolabDelegate'] = [];
		$i = 0;
		while (isset($_POST['delegate' . $i])) {
			if (isset($_POST['delDelegate' . $i])) {
				$i++;
				continue;
			}
			$this->attributes['kolabDelegate'][] = $_POST['delegate' . $i];
			$i++;
		}
		// check new delegate
		if (isset($_POST['addDelegate']) && ($_POST['delegate'] != "")) {
			$this->attributes['kolabDelegate'][] = $_POST['delegate'];
		}
		$this->attributes['kolabDelegate'] = array_unique($this->attributes['kolabDelegate']);
		return $errors;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$messages = [];
		$delegates = searchLDAPByAttribute(null, null, 'inetOrgPerson', ['dn'], ['user']);
		for ($d = 0; $d < sizeof($delegates); $d++) {
			if (isset($delegates[$d]['dn'])) {
				$delegates[$d] = $delegates[$d]['dn'];
			}
		}
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// add object classes
			if (!in_array('kolabSharedFolder', $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = 'kolabSharedFolder';
			}
			if (!in_array('mailrecipient', $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = 'mailrecipient';
			}
			// cn
			$partialAccounts[$i]['cn'] = $rawAccounts[$i][$ids['kolabSharedFolder_cn']];
			// mail
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabSharedFolder_mail', 'mail',
				'email', $this->messages['mail'][1], $messages);
			// target folder
			$partialAccounts[$i]['kolabTargetFolder'] = $rawAccounts[$i][$ids['kolabSharedFolder_kolabTargetFolder']];
			// folder type
			$partialAccounts[$i]['kolabFolderType'] = $rawAccounts[$i][$ids['kolabSharedFolder_kolabFolderType']];
			// allowed recipients
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabSharedFolder_kolabAllowSMTPRecipient', 'kolabAllowSMTPRecipient', 'kolabEmailPrefix', $this->messages['kolabAllowSMTPRecipient'][1], $messages, '/;[ ]*/');
			// allowed senders
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabSharedFolder_kolabAllowSMTPSender', 'kolabAllowSMTPSender', 'kolabEmailPrefix', $this->messages['kolabAllowSMTPSender'][1], $messages, '/;[ ]*/');
			// add mail aliases
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'kolabSharedFolder_aliases', 'alias', 'email', $this->messages['alias'][1], $messages, '/,[ ]*/');
			// add delegates
			if ($rawAccounts[$i][$ids['kolabSharedFolder_delegates']] != "") {
				$newDelegates = explode(';', $rawAccounts[$i][$ids['kolabSharedFolder_delegates']]);
				// check format
				for ($d = 0; $d < sizeof($newDelegates); $d++) {
					if (in_array($newDelegates[$d], $delegates)) {
						$partialAccounts[$i]['kolabDelegate'][] = $newDelegates[$d];
					}
					// invalid format
					else {
						$errMsg = $this->messages['delegate'][0];
						array_push($errMsg, [$i, $newDelegates[$d]]);
						$messages[] = $errMsg;
					}
				}
			}
		}
		return $messages;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	function get_pdfEntries($pdfKeys, $typeId) {
		$return = [];
		$this->addSimplePDFField($return, 'cn', _('Name'));
		$this->addSimplePDFField($return, 'mail', _('Email address'));
		$this->addSimplePDFField($return, 'kolabTargetFolder', _('Target IMAP folder'));
		$this->addSimplePDFField($return, 'kolabAllowSMTPRecipient', _('Allowed recipients'));
		$this->addSimplePDFField($return, 'kolabAllowSMTPSender', _('Allowed senders'));
		$this->addSimplePDFField($return, 'aliases', _('Email aliases'), 'alias');
		$this->addSimplePDFField($return, 'delegate', _('Delegates'), 'kolabDelegate', '; ');
		if (!empty($this->attributes['kolabFolderType'])) {
			$type = $this->attributes['kolabFolderType'][0];
			$typeList = array_flip($this->folderTypes);
			if (isset($typeList[$type])) {
				$type = $typeList[$type];
			}
			$this->addPDFKeyValue($return, 'kolabFolderType', _('Type'), $type);
		}
		return $return;
	}

	/**
	 * @inheritDoc
	 */
	public function getListAttributeDescriptions(ConfiguredType $type): array {
		return [
			"cn" => _('Name'),
			"mail" => _('Email address'),
			"kolabdelegate" => _('Delegates'),
			'alias' => _('Email aliases'),
		];
	}

	/**
	 * @inheritDoc
	 */
	public function getListRenderFunction(string $attributeName): ?callable {
		if ($attributeName === 'mail') {
			return function(array $entry, string $attribute): ?htmlElement {
				$group = new htmlGroup();
				if (isset($entry[$attribute][0]) && ($entry[$attribute][0] != '')) {
					for ($i = 0; $i < sizeof($entry[$attribute]); $i++) {
						if ($i > 0) {
							$group->addElement(new htmlOutputText(", "));
						}
						$group->addElement(new htmlLink($entry[$attribute][$i], "mailto:" . $entry[$attribute][$i]));
					}
				}
				return $group;
			};
		}
		return null;
	}

}
